#include "graphics_pipeline.hpp"
#include "vulkan_common/device.hpp"
#include "descriptor_pool.hpp"
#include "render_base.hpp"
#include "scheduler.hpp"
#include "shader_notify.hpp"
#include "pipeline_statistics.hpp"
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include "render_pass.hpp"
#include "pipeline_helper.hpp"
#if defined(_MSC_VER) && defined(NDEBUG)
#define LAMBDA_FORCEINLINE [[msvc::forceinline]]
#else
#define LAMBDA_FORCEINLINE
#endif
constexpr size_t MAX_IMAGE_ELEMENTS = 64;
namespace render::vulkan {
namespace {
using boost::container::small_vector;
using boost::container::static_vector;
auto MakeBuilder(const Device& device, std::span<const shader::Info> infos)
    -> pipeline::DescriptorLayoutBuilder {
    pipeline::DescriptorLayoutBuilder builder{device};
    for (size_t index = 0; index < infos.size(); ++index) {
        static constexpr std::array stages{
            vk::ShaderStageFlagBits::eVertex,
            vk::ShaderStageFlagBits::eTessellationControl,
            vk::ShaderStageFlagBits::eTessellationEvaluation,
            vk::ShaderStageFlagBits::eGeometry,
            vk::ShaderStageFlagBits::eFragment,
        };
        builder.Add(infos[index], stages.at(index));
    }
    return builder;
}
template <class StencilFace>
vk::StencilOpState GetStencilFaceState(const StencilFace& face) {
    return vk::StencilOpState{vk::StencilOp::eKeep,
                              vk::StencilOp::eReplace,
                              vk::StencilOp::eKeep,
                              vk::CompareOp::eAlways,
                              0xFF,
                              0xFF,
                              1};
}
bool SupportsPrimitiveRestart(vk::PrimitiveTopology topology) {
    static constexpr std::array unsupported_topologies{
        vk::PrimitiveTopology::ePointList,
        vk::PrimitiveTopology::eLineList,
        vk::PrimitiveTopology::eTriangleList,
        vk::PrimitiveTopology::eLineListWithAdjacency,
        vk::PrimitiveTopology::eTriangleListWithAdjacency,
        vk::PrimitiveTopology::ePatchList,
        // VK_PRIMITIVE_TOPOLOGY_QUAD_LIST_EXT,
    };
    return std::ranges::find(unsupported_topologies, topology) == unsupported_topologies.end();
}

bool IsLine(vk::PrimitiveTopology topology) {
    static constexpr std::array line_topologies{
        vk::PrimitiveTopology::eLineList, vk::PrimitiveTopology::eLineStrip,
        // VK_PRIMITIVE_TOPOLOGY_LINE_LOOP_EXT,
    };
    return std::ranges::find(line_topologies, topology) == line_topologies.end();
}

/**
* @brief VkViewportSwizzleNV 是一个 Vulkan
* 扩展，它允许开发者在渲染过程中对视口（viewport）坐标进行重新排列（swizzle）。这个扩展通过
* VkViewportSwizzleNV 结构体来定义视口坐标的重新排列操作。
   VkViewportCoordinateSwizzleNV 枚举定义了可能的重新排列操作值，例如：
   VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_X_NV：将 x 成分替换为正 x 坐标。
   VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_X_NV：将 x 成分替换为负 x 坐标。
   VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_Y_NV：将 y 成分替换为正 y 坐标。
   VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_Y_NV：将 y 成分替换为负 y 坐标。
   VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_Z_NV：将 z 成分替换为正 z 坐标。
   VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_Z_NV：将 z 成分替换为负 z 坐标。
   VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_W_NV：将 w 成分替换为正 w 坐标。
   VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_W_NV：将 w 成分替换为负 w 坐标。
   *
* @param swizzle
* @return vk::ViewportSwizzleNV
*/
auto UnpackViewportSwizzle(u16 swizzle) -> vk::ViewportSwizzleNV { return vk::ViewportSwizzleNV{}; }

auto MakeRenderPassKey() -> RenderPassKey {
    RenderPassKey key;
    key.color_formats[0] = surface::PixelFormat::B8G8R8A8_UNORM;
    key.depth_format = surface::PixelFormat::D32_FLOAT;
    return key;
}

template <typename Spec>
auto Passes(const std::array<ShaderModule, NUM_STAGES>& modules,
            const std::array<shader::Info, NUM_STAGES>& stage_infos) -> bool {
    for (size_t stage = 0; stage < NUM_STAGES; ++stage) {
        if (!Spec::enabled_stages[stage] && modules[stage]) {
            return false;
        }
        const auto& info{stage_infos[stage]};
        if constexpr (!Spec::has_storage_buffers) {
            if (!info.storage_buffers_descriptors.empty()) {
                return false;
            }
        }
        if constexpr (!Spec::has_texture_buffers) {
            if (!info.texture_buffer_descriptors.empty()) {
                return false;
            }
        }
        if constexpr (!Spec::has_image_buffers) {
            if (!info.image_buffer_descriptors.empty()) {
                return false;
            }
        }
        if constexpr (!Spec::has_images) {
            if (!info.image_descriptors.empty()) {
                return false;
            }
        }
    }
    return true;
}

using ConfigureFuncPtr = void (*)(GraphicsPipeline*, bool);
template <typename Spec, typename... Specs>
ConfigureFuncPtr FindSpec(const std::array<ShaderModule, NUM_STAGES>& modules,
                          const std::array<shader::Info, NUM_STAGES>& stage_infos) {
    if constexpr (sizeof...(Specs) > 0) {
        if (!Passes<Spec>(modules, stage_infos)) {
            return FindSpec<Specs...>(modules, stage_infos);
        }
    }
    return GraphicsPipeline::MakeConfigureSpecFunc<Spec>();
}
struct SimpleVertexFragmentSpec {
        static constexpr std::array<bool, 5> enabled_stages{true, false, false, false, true};
        static constexpr bool has_storage_buffers = false;
        static constexpr bool has_texture_buffers = false;
        static constexpr bool has_image_buffers = false;
        static constexpr bool has_images = false;
};

struct SimpleVertexSpec {
        static constexpr std::array<bool, 5> enabled_stages{true, false, false, false, false};
        static constexpr bool has_storage_buffers = false;
        static constexpr bool has_texture_buffers = false;
        static constexpr bool has_image_buffers = false;
        static constexpr bool has_images = false;
};

struct SimpleStorageSpec {
        static constexpr std::array<bool, 5> enabled_stages{true, false, false, false, true};
        static constexpr bool has_storage_buffers = true;
        static constexpr bool has_texture_buffers = false;
        static constexpr bool has_image_buffers = false;
        static constexpr bool has_images = false;
};

struct SimpleImageSpec {
        static constexpr std::array<bool, 5> enabled_stages{true, false, false, false, true};
        static constexpr bool has_storage_buffers = false;
        static constexpr bool has_texture_buffers = false;
        static constexpr bool has_image_buffers = false;
        static constexpr bool has_images = true;
};

struct DefaultSpec {
        static constexpr std::array<bool, 5> enabled_stages{true, true, true, true, true};
        static constexpr bool has_storage_buffers = true;
        static constexpr bool has_texture_buffers = true;
        static constexpr bool has_image_buffers = true;
        static constexpr bool has_images = true;
};

ConfigureFuncPtr ConfigureFunc(const std::array<ShaderModule, NUM_STAGES>& modules,
                               const std::array<shader::Info, NUM_STAGES>& infos) {
    return FindSpec<SimpleVertexSpec, SimpleVertexFragmentSpec, SimpleStorageSpec, SimpleImageSpec,
                    DefaultSpec>(modules, infos);
}
}  // namespace

bool GraphicsPipelineCacheKey::operator==(const GraphicsPipelineCacheKey& rhs) const noexcept {
    return std::memcmp(&rhs, this, Size()) == 0;
}

GraphicsPipeline::GraphicsPipeline(
    scheduler::Scheduler& scheduler, PipelineCache& pipeline_cache_, ShaderNotify* shader_notify,
    const Device& device, resource::DescriptorPool& descriptor_pool,
    GuestDescriptorQueue& guest_descriptor_queue_, common::ThreadWorker* worker_thread,
    pipeline::PipelineStatistics* pipeline_statistics, RenderPassCache& render_pass_cache,
    const GraphicsPipelineCacheKey& key, std::array<ShaderModule, NUM_STAGES> stages,
    const std::array<const shader::Info*, NUM_STAGES>& infos)
    : key_{key},
      device_{device},
      pipeline_cache(pipeline_cache_),
      scheduler_{scheduler},
      guest_descriptor_queue_{guest_descriptor_queue_},
      spv_modules_{std::move(stages)} {
    if (shader_notify) {
        shader_notify->MarkShaderBuilding();
    }
    for (size_t stage = 0; stage < NUM_STAGES; ++stage) {
        const shader::Info* const info{infos[stage]};
        if (!info) {
            continue;
        }
        stage_infos[stage] = *info;
        enabled_uniform_buffer_masks[stage] = info->constant_buffer_mask;
        std::ranges::copy(info->constant_buffer_used_sizes, uniform_buffer_sizes[stage].begin());
        num_textures += shader::NumDescriptors(info->texture_descriptors);
    }
    auto func{[this, shader_notify, &render_pass_cache, &descriptor_pool, pipeline_statistics] {
        pipeline::DescriptorLayoutBuilder builder{MakeBuilder(device_, stage_infos)};
        uses_push_descriptor = builder.CanUsePushDescriptor();
        descriptor_set_layout = builder.CreateDescriptorSetLayout(uses_push_descriptor);
        if (!uses_push_descriptor) {
            // descriptor_allocator_ = descriptor_pool.Allocator(*descriptor_set_layout,
            // stage_infos);
        }
        const VkDescriptorSetLayout set_layout{*descriptor_set_layout};
        pipeline_layout = builder.CreatePipelineLayout(set_layout);
        descriptor_update_template =
            builder.CreateTemplate(set_layout, *pipeline_layout, uses_push_descriptor);

        // const vk::RenderPass render_pass = render_pass_cache.get(MakeRenderPassKey(key.state));
        // validate();
        // makePipeline(render_pass);
        // if (pipeline_statistics) {
        //     pipeline_statistics->Collect(*pipeline);
        // }

        std::scoped_lock lock{build_mutex};
        is_built = true;
        build_condvar.notify_one();
        if (shader_notify) {
            shader_notify->MarkShaderComplete();
        }
    }};
    if (worker_thread) {
        worker_thread->QueueWork(std::move(func));
    } else {
        func();
    }
    configure_func = ConfigureFunc(spv_modules_, stage_infos);
}

void GraphicsPipeline::AddTransition(GraphicsPipeline* transition) {
    transition_keys.push_back(transition->key_);
    transitions.push_back(transition);
}

template <typename Spec>
void GraphicsPipeline::configureImpl(bool is_indexed) {}

void GraphicsPipeline::ConfigureDraw(const pipeline::RescalingPushConstant& rescaling,
                                     const pipeline::RenderAreaPushConstant& render_area) {
    // scheduler_.requestRenderpass(texture_cache.GetFramebuffer());

    // if (!is_built.load(std::memory_order::relaxed)) {
    //     // Wait for the pipeline to be built
    //     scheduler.Record([this](vk::CommandBuffer) {
    //         std::unique_lock lock{build_mutex};
    //         build_condvar.wait(lock, [this] { return is_built.load(std::memory_order::relaxed);
    //         });
    //     });
    // }
    // const bool is_rescaling{texture_cache.IsRescaling()};
    // const bool update_rescaling{scheduler_.updateRescaling(is_rescaling)};
    // const bool bind_pipeline{scheduler_.updateGraphicsPipeline(this)};
    // const void* const descriptor_data{guest_descriptor_queue_.UpdateData()};
    // scheduler.Record([this, descriptor_data, bind_pipeline, rescaling_data = rescaling.Data(),
    //                   is_rescaling, update_rescaling,
    //                   uses_render_area = render_area.uses_render_area,
    //                   render_area_data = render_area.words](vk::CommandBuffer cmdbuf) {
    //     if (bind_pipeline) {
    //         cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
    //     }
    //     cmdbuf.PushConstants(*pipeline_layout, VK_SHADER_STAGE_ALL_GRAPHICS,
    //                          RESCALING_LAYOUT_WORDS_OFFSET, sizeof(rescaling_data),
    //                          rescaling_data.data());
    //     if (update_rescaling) {
    //         const f32 config_down_factor{Settings::values.resolution_info.down_factor};
    //         const f32 scale_down_factor{is_rescaling ? config_down_factor : 1.0f};
    //         cmdbuf.PushConstants(*pipeline_layout, VK_SHADER_STAGE_ALL_GRAPHICS,
    //                              RESCALING_LAYOUT_DOWN_FACTOR_OFFSET, sizeof(scale_down_factor),
    //                              &scale_down_factor);
    //     }
    //     if (uses_render_area) {
    //         cmdbuf.PushConstants(*pipeline_layout, VK_SHADER_STAGE_ALL_GRAPHICS,
    //                              RENDERAREA_LAYOUT_OFFSET, sizeof(render_area_data),
    //                              &render_area_data);
    //     }
    //     if (!descriptor_set_layout) {
    //         return;
    //     }
    //     if (uses_push_descriptor) {
    //         cmdbuf.PushDescriptorSetWithTemplateKHR(*descriptor_update_template,
    //         *pipeline_layout,
    //                                                 0, descriptor_data);
    //     } else {
    //         const VkDescriptorSet descriptor_set{descriptor_allocator.Commit()};
    //         const vk::Device& dev{device.GetLogical()};
    //         dev.UpdateDescriptorSet(descriptor_set, *descriptor_update_template,
    //         descriptor_data); cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
    //         *pipeline_layout, 0,
    //                                   descriptor_set, nullptr);
    //     }
    // });
}

void GraphicsPipeline::validate() {
    size_t num_images{};
    for (const auto& info : stage_infos) {
        num_images += shader::NumDescriptors(info.texture_buffer_descriptors);
        num_images += shader::NumDescriptors(info.image_buffer_descriptors);
        num_images += shader::NumDescriptors(info.texture_descriptors);
        num_images += shader::NumDescriptors(info.image_descriptors);
    }
    assert(num_images <= MAX_IMAGE_ELEMENTS);
}

}  // namespace render::vulkan