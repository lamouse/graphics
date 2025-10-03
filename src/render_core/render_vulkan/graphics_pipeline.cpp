#include "graphics_pipeline.hpp"
#include "vulkan_common/device.hpp"
#include "descriptor_pool.hpp"
#include "scheduler.hpp"
#include "common/assert.hpp"
#include "shader_notify.hpp"
#include "pipeline_statistics.hpp"
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include "render_pass.hpp"
#include "pipeline_helper.hpp"
#include "shader_tools/stage.h"
#include <gsl/gsl>
#include <farmhash.h>
#if defined(_MSC_VER) && defined(NDEBUG)
#define LAMBDA_FORCEINLINE [[msvc::forceinline]]
#else
#define LAMBDA_FORCEINLINE
#endif
constexpr size_t MAX_IMAGE_ELEMENTS = 64;
namespace render::vulkan {
namespace {

auto SupportsPrimitiveRestart(VkPrimitiveTopology topology) -> bool {
    static constexpr std::array unsupported_topologies{
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
        // VK_PRIMITIVE_TOPOLOGY_QUAD_LIST_EXT,
    };
    return std::ranges::find(unsupported_topologies, topology) == unsupported_topologies.end();
}

auto ShaderStage(uint32_t stage) -> VkShaderStageFlagBits {
    auto shader_stage = shader::StageFromIndex(stage);
    switch (shader_stage) {
        case shader::Stage::VertexA:
        case shader::Stage::VertexB:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case shader::Stage::TessellationControl:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case shader::Stage::TessellationEval:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case shader::Stage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case shader::Stage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case shader::Stage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
    }
    SPDLOG_WARN("... Invalid shader stage={}", stage);
    return {};
}

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
auto GetStencilFaceState(const StencilFace& face) -> vk::StencilOpState {
    return vk::StencilOpState{vk::StencilOp::eKeep,
                              vk::StencilOp::eReplace,
                              vk::StencilOp::eKeep,
                              vk::CompareOp::eAlways,
                              0xFF,
                              0xFF,
                              1};
}
auto SupportsPrimitiveRestart(vk::PrimitiveTopology topology) -> bool {
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

auto IsLine(vk::PrimitiveTopology topology) -> bool {
    static constexpr std::array line_topologies{
        vk::PrimitiveTopology::eLineList, vk::PrimitiveTopology::eLineStrip
        // VK_PRIMITIVE_TOPOLOGY_LINE_LOOP_EXT,
    };
    return std::ranges::find(line_topologies, topology) == line_topologies.end();
}

auto MsaaMode(MsaaMode msaa_mode) -> VkSampleCountFlagBits {
    switch (msaa_mode) {
        case MsaaMode::Msaa1x1:
            return VK_SAMPLE_COUNT_1_BIT;
        case MsaaMode::Msaa2x1:
        case MsaaMode::Msaa2x1_D3D:
            return VK_SAMPLE_COUNT_2_BIT;
        case MsaaMode::Msaa2x2:
        case MsaaMode::Msaa2x2_VC4:
        case MsaaMode::Msaa2x2_VC12:
            return VK_SAMPLE_COUNT_4_BIT;
        case MsaaMode::Msaa4x2:
        case MsaaMode::Msaa4x2_D3D:
        case MsaaMode::Msaa4x2_VC8:
        case MsaaMode::Msaa4x2_VC24:
            return VK_SAMPLE_COUNT_8_BIT;
        case MsaaMode::Msaa4x4:
            return VK_SAMPLE_COUNT_16_BIT;
        default:
            ASSERT(false &&
                   fmt::format("Invalid msaa_mode={}", static_cast<int>(msaa_mode)).c_str());
            return VK_SAMPLE_COUNT_1_BIT;
    }
}

auto MakeRenderPassKey(const FixedPipelineState& state) -> RenderPassKey {
    RenderPassKey key;
    std::ranges::transform(state.color_formats, key.color_formats.begin(),
                           [](auto format) { return format; });
    if (state.depth_enabled != 0) {
        key.depth_format = state.depth_format;
    } else {
        key.depth_format = surface::PixelFormat::Invalid;
    }
    key.samples = static_cast<vk::SampleCountFlagBits>(MsaaMode(state.msaa_mode));
    return key;
}

template <typename Spec>
auto Passes(const std::array<ShaderModule, buffer::NUM_STAGES>& modules,
            const std::array<shader::Info, buffer::NUM_STAGES>& stage_infos) -> bool {
    for (size_t stage = 0; stage < buffer::NUM_STAGES; ++stage) {
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
ConfigureFuncPtr FindSpec(const std::array<ShaderModule, buffer::NUM_STAGES>& modules,
                          const std::array<shader::Info, buffer::NUM_STAGES>& stage_infos) {
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

ConfigureFuncPtr ConfigureFunc(const std::array<ShaderModule, buffer::NUM_STAGES>& modules,
                               const std::array<shader::Info, buffer::NUM_STAGES>& infos) {
    return FindSpec<SimpleVertexSpec, SimpleVertexFragmentSpec, SimpleStorageSpec, SimpleImageSpec,
                    DefaultSpec>(modules, infos);
}
}  // namespace

auto GraphicsPipelineCacheKey::operator==(const GraphicsPipelineCacheKey& rhs) const noexcept
    -> bool {
    return std::memcmp(&rhs, this, Size()) == 0;
}

auto GraphicsPipelineCacheKey::Hash() const noexcept -> size_t {
    const u64 hash = NAMESPACE_FOR_HASH_FUNCTIONS::Fingerprint64(reinterpret_cast<const char*>(this), Size());
    return static_cast<size_t>(hash);
}

GraphicsPipeline::GraphicsPipeline(
    scheduler::Scheduler& scheduler, VulkanPipelineCache& pipeline_cache_,
    ShaderNotify* shader_notify, const Device& device, resource::DescriptorPool& descriptor_pool,
    GuestDescriptorQueue& guest_descriptor_queue_, common::ThreadWorker* worker_thread,
    pipeline::PipelineStatistics* pipeline_statistics, RenderPassCache& render_pass_cache,
    const GraphicsPipelineCacheKey& key, TextureCache& texture_cache_, BufferCache& buffer_cache_,
    std::array<ShaderModule, NUM_STAGES> stages,
    const std::array<const shader::Info*, NUM_STAGES>& infos, DynamicFeatures dynamic_)
    : key_{key},
      device_{device},
      pipeline_cache(pipeline_cache_),
      scheduler_{scheduler},
      guest_descriptor_queue_{guest_descriptor_queue_},
      spv_modules_{std::move(stages)},
      dynamic(dynamic_),
      texture_cache(texture_cache_),
      buffer_cache(buffer_cache_) {
    if (shader_notify) {
        shader_notify->MarkShaderBuilding();
    }
    for (u32 stage = 0; stage < NUM_STAGES; ++stage) {
        const shader::Info* const info{gsl::at(infos, stage)};
        if (!info) {
            continue;
        }
        gsl::at(stage_infos, stage) = *info;
        num_textures += shader::NumDescriptors(info->texture_descriptors);
    }
    auto func{[this, shader_notify, &render_pass_cache, &descriptor_pool, pipeline_statistics] {
        pipeline::DescriptorLayoutBuilder builder{MakeBuilder(device_, stage_infos)};
        uses_push_descriptor = builder.CanUsePushDescriptor();
        descriptor_set_layout = builder.CreateDescriptorSetLayout(uses_push_descriptor);
        if (!uses_push_descriptor) {
            descriptor_allocator = descriptor_pool.allocator(*descriptor_set_layout, stage_infos);
        }
        const vk::DescriptorSetLayout set_layout{*descriptor_set_layout};
        pipeline_layout = builder.CreatePipelineLayout(set_layout);
        descriptor_update_template =
            builder.CreateTemplate(set_layout, *pipeline_layout, uses_push_descriptor);

        const vk::RenderPass render_pass = render_pass_cache.get(MakeRenderPassKey(key_.state));
        validate();
        makePipeline(render_pass);
        if (pipeline_statistics) {
            pipeline_statistics->Collect(*pipeline);
        }

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
void GraphicsPipeline::configureImpl(bool is_indexed) {


    ConfigureDraw();
}

void GraphicsPipeline::ConfigureDraw() {
    scheduler_.requestRenderPass(texture_cache.getFramebuffer());

    if (!is_built.load(std::memory_order::relaxed)) {
        // Wait for the pipeline to be built
        scheduler_.record([this](vk::CommandBuffer) {
            std::unique_lock lock{build_mutex};
            build_condvar.wait(lock, [this] { return is_built.load(std::memory_order::relaxed); });
        });
    }
    const bool bind_pipeline{scheduler_.updateGraphicsPipeline(this)};
    const void* const descriptor_data{guest_descriptor_queue_.UpdateData()};
    scheduler_.record([this, descriptor_data, bind_pipeline](vk::CommandBuffer cmdbuf) {
        if (bind_pipeline) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
        }
        if (!descriptor_set_layout) {
            return;
        }
        if (uses_push_descriptor) {
            cmdbuf.pushDescriptorSetWithTemplateKHR(*descriptor_update_template, *pipeline_layout,
                                                    0, descriptor_data,
                                                    device_.logical().getDispatchLoaderDynamic());
        } else {
            const vk::DescriptorSet descriptor_set{descriptor_allocator.commit()};
            device_.getLogical().updateDescriptorSetWithTemplate(
                descriptor_set, *descriptor_update_template, descriptor_data);
            cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0,
                                      descriptor_set, nullptr);
        }
    });
}
GraphicsPipeline::~GraphicsPipeline() = default;
void GraphicsPipeline::validate() {
    size_t num_images{};
    for (const auto& info : stage_infos) {
        num_images += shader::NumDescriptors(info.texture_buffer_descriptors);
        num_images += shader::NumDescriptors(info.image_buffer_descriptors);
        num_images += shader::NumDescriptors(info.texture_descriptors);
        num_images += shader::NumDescriptors(info.image_descriptors);
    }
    ASSERT(num_images <= MAX_IMAGE_ELEMENTS);
}

void GraphicsPipeline::makePipeline(vk::RenderPass render_pass) {
    static_vector<VkVertexInputBindingDescription, 32> vertex_bindings;
    static_vector<VkVertexInputBindingDivisorDescriptionEXT, 32> vertex_binding_divisors;
    static_vector<VkVertexInputAttributeDescription, 32> vertex_attributes;

    VkPipelineVertexInputStateCreateInfo vertex_input_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = static_cast<u32>(vertex_bindings.size()),
        .pVertexBindingDescriptions = vertex_bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<u32>(vertex_attributes.size()),
        .pVertexAttributeDescriptions = vertex_attributes.data(),
    };
    const VkPipelineVertexInputDivisorStateCreateInfoEXT input_divisor_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT,
        .pNext = nullptr,
        .vertexBindingDivisorCount = static_cast<u32>(vertex_binding_divisors.size()),
        .pVertexBindingDivisors = vertex_binding_divisors.data(),
    };
    if (!vertex_binding_divisors.empty()) {
        // vertex_input_ci.pNext = &input_divisor_ci;
    }
    const bool has_tess_stages = spv_modules_[1] || spv_modules_[2];
    auto input_assembly_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    const VkPipelineInputAssemblyStateCreateInfo input_assembly_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = input_assembly_topology,
        .primitiveRestartEnable =
            (input_assembly_topology != VK_PRIMITIVE_TOPOLOGY_PATCH_LIST &&
             device_.IsTopologyListPrimitiveRestartSupported()) ||
                    SupportsPrimitiveRestart(input_assembly_topology) ||
                    (input_assembly_topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST &&
                     device_.IsPatchListPrimitiveRestartSupported())
                ? VK_TRUE
                : VK_FALSE,
    };
    const VkPipelineTessellationStateCreateInfo tessellation_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .patchControlPoints = 4,
    };
    VkPipelineViewportSwizzleStateCreateInfoNV swizzle_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewportSwizzles = nullptr,
    };
    VkPipelineViewportDepthClipControlCreateInfoEXT ndc_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_DEPTH_CLIP_CONTROL_CREATE_INFO_EXT,
        .pNext = nullptr,
        .negativeOneToOne = VK_FALSE,
    };
    VkPipelineViewportStateCreateInfo viewport_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };
    if (device_.IsNvViewportSwizzleSupported()) {
        //swizzle_ci.pNext = std::exchange(viewport_ci.pNext, &swizzle_ci);
    }
    if (device_.IsExtDepthClipControlSupported()) {
        ndc_info.pNext = std::exchange(viewport_ci.pNext, &ndc_info);
    }
    VkPipelineRasterizationStateCreateInfo rasterization_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };
    VkPipelineRasterizationLineStateCreateInfoEXT line_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT,
        .pNext = nullptr,
        .lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT,
        .stippledLineEnable = VK_FALSE,
        .lineStippleFactor = 0,
        .lineStipplePattern = 0,
    };
    VkPipelineRasterizationConservativeStateCreateInfoEXT conservative_raster{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT,
        .extraPrimitiveOverestimationSize = 0.0f,
    };
    VkPipelineRasterizationProvokingVertexStateCreateInfoEXT provoking_vertex{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_PROVOKING_VERTEX_STATE_CREATE_INFO_EXT,
        .pNext = nullptr,
        .provokingVertexMode = VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT,
    };
    if (IsLine(static_cast<vk::PrimitiveTopology>(input_assembly_topology)) &&
        device_.IsExtLineRasterizationSupported()) {
        line_state.pNext = std::exchange(rasterization_ci.pNext, &line_state);
    }
    if (device_.IsExtConservativeRasterizationSupported()) {
        conservative_raster.pNext = std::exchange(rasterization_ci.pNext, &conservative_raster);
    }
    if (device_.IsExtProvokingVertexSupported()) {
        provoking_vertex.pNext = std::exchange(rasterization_ci.pNext, &provoking_vertex);
    }

    const VkPipelineMultisampleStateCreateInfo multisample_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    const VkPipelineDepthStencilStateCreateInfo depth_stencil_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = device_.IsDepthBoundsSupported(),
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 0.0f,
    };
    static_vector<VkPipelineColorBlendAttachmentState, 8> cb_attachments;
    const size_t num_attachments{1};
    for (size_t index = 0; index < num_attachments; ++index) {
        static constexpr std::array mask_table{
            VK_COLOR_COMPONENT_R_BIT,
            VK_COLOR_COMPONENT_G_BIT,
            VK_COLOR_COMPONENT_B_BIT,
            VK_COLOR_COMPONENT_A_BIT,
        };
        VkColorComponentFlags write_mask{};
        for (size_t i = 0; i < mask_table.size(); ++i) {
            write_mask |= mask_table[i];
        }
        cb_attachments.push_back({
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC1_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = write_mask,
        });
    }
    const VkPipelineColorBlendStateCreateInfo color_blend_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<u32>(cb_attachments.size()),
        .pAttachments = cb_attachments.data(),
        .blendConstants = {},
    };
    static_vector<VkDynamicState, 28> dynamic_states{
        VK_DYNAMIC_STATE_VIEWPORT,           VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_BIAS,         VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS,       VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
        VK_DYNAMIC_STATE_STENCIL_WRITE_MASK, VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        VK_DYNAMIC_STATE_LINE_WIDTH,
    };
    if (dynamic.has_extended_dynamic_state) {
        static constexpr std::array extended{
            VK_DYNAMIC_STATE_CULL_MODE_EXT,
            VK_DYNAMIC_STATE_FRONT_FACE_EXT,
            VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT,
            VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT,
            VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT,
            VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT,
            VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT,
            VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT,
            VK_DYNAMIC_STATE_STENCIL_OP_EXT,
        };
        if (dynamic.has_dynamic_vertex_input) {
            dynamic_states.push_back(VK_DYNAMIC_STATE_VERTEX_INPUT_EXT);
        }
        dynamic_states.insert(dynamic_states.end(), extended.begin(), extended.end());
        if (dynamic.has_extended_dynamic_state_2) {
            static constexpr std::array extended2{
                VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT,
                VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE_EXT,
                VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT,
            };
            dynamic_states.insert(dynamic_states.end(), extended2.begin(), extended2.end());
        }

        if (dynamic.has_extended_dynamic_state_2_extra) {
            dynamic_states.push_back(VK_DYNAMIC_STATE_LOGIC_OP_EXT);
        }
        if (dynamic.has_extended_dynamic_state_3_blend) {
            static constexpr std::array extended3{
                VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT,
                VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT,
                VK_DYNAMIC_STATE_COLOR_WRITE_MASK_EXT,
            };
            dynamic_states.insert(dynamic_states.end(), extended3.begin(), extended3.end());
        }
        if (dynamic.has_extended_dynamic_state_3_enables) {
            static constexpr std::array extended3{
                VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT,
                VK_DYNAMIC_STATE_LOGIC_OP_ENABLE_EXT,
            };
            dynamic_states.insert(dynamic_states.end(), extended3.begin(), extended3.end());
        }
    }
    const VkPipelineDynamicStateCreateInfo dynamic_state_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<u32>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };
    [[maybe_unused]] const VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroup_size_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT,
        .pNext = nullptr,
        .requiredSubgroupSize = GUEST_WARP_SIZE,
    };
    static_vector<VkPipelineShaderStageCreateInfo, 5> shader_stages;
    for (uint32_t stage = 0; stage < 5; ++stage) {
        if (!spv_modules_[stage]) {
            continue;
        }
        [[maybe_unused]] auto& stage_ci =
            shader_stages.emplace_back(VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = ShaderStage(stage),
                .module = *spv_modules_[stage],
                .pName = "main",
                .pSpecializationInfo = nullptr,
            });
    }
    VkPipelineCreateFlags flags{};
    if (device_.IsKhrPipelineExecutablePropertiesEnabled()) {
        flags |= VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
    }
    VkGraphicsPipelineCreateInfo pipeline_ci{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .stageCount = static_cast<u32>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pTessellationState = &tessellation_ci,
        .pViewportState = &viewport_ci,
        .pRasterizationState = &rasterization_ci,
        .pMultisampleState = &multisample_ci,
        .pDepthStencilState = &depth_stencil_ci,
        .pColorBlendState = &color_blend_ci,
        .pDynamicState = &dynamic_state_ci,
        .layout = *pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = 0,
    };
    pipeline =
        device_.logical().createPipeline(static_cast<vk::GraphicsPipelineCreateInfo>(pipeline_ci));
}
}  // namespace render::vulkan