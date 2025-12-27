#include "graphics_pipeline.hpp"
#include "format_to_vk.hpp"
#include "vulkan_common/device.hpp"
#include "descriptor_pool.hpp"
#include "scheduler.hpp"
#include "common/assert.hpp"
#include "common/settings.hpp"
#include "shader_notify.hpp"
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include "render_pass.hpp"
#include "pipeline_helper.hpp"
#include "shader_tools/stage.h"
#include <gsl/gsl>
#include <xxhash.h>
#if defined(_MSC_VER) && defined(NDEBUG)
#define LAMBDA_FORCEINLINE [[msvc::forceinline]]
#else
#define LAMBDA_FORCEINLINE
#endif
constexpr size_t MAX_IMAGE_ELEMENTS = 64;
constexpr size_t NUM_RENDER_TARGETS = 8;
namespace render::vulkan {
namespace {

auto ShaderStage(uint32_t stage) -> vk::ShaderStageFlagBits {
    auto shader_stage = shader::StageFromIndex(stage);
    switch (shader_stage) {
        case shader::Stage::Vertex:
            return vk::ShaderStageFlagBits::eVertex;
        case shader::Stage::TessellationControl:
            return vk::ShaderStageFlagBits::eTessellationControl;
        case shader::Stage::TessellationEval:
            return vk::ShaderStageFlagBits::eTessellationEvaluation;
        case shader::Stage::Geometry:
            return vk::ShaderStageFlagBits::eGeometry;
        case shader::Stage::Fragment:
            return vk::ShaderStageFlagBits::eFragment;
        case shader::Stage::Compute:
            return vk::ShaderStageFlagBits::eCompute;
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
    return vk::StencilOpState{StencilOp(face.fail),
                              StencilOp(face.pass),
                              StencilOp(face.depthFail),
                              ComparisonOp(face.compare),
                              0xFF,
                              0xFF,
                              1};
}

auto NumAttachments(const FixedPipelineState& state) -> size_t {
    size_t num{};
    for (size_t index = 0; index < NUM_RENDER_TARGETS; ++index) {
        const auto format(state.color_formats[index]);
        if (format != surface::PixelFormat::Invalid) {
            num = index + 1;
        }
    }
    return num;
}

auto SupportsPrimitiveRestart(vk::PrimitiveTopology topology) -> bool {
    static constexpr std::array unsupported_topologies{
        vk::PrimitiveTopology::ePointList,
        vk::PrimitiveTopology::eLineList,
        vk::PrimitiveTopology::eTriangleList,
        vk::PrimitiveTopology::eLineListWithAdjacency,
        vk::PrimitiveTopology::eTriangleListWithAdjacency,
        vk::PrimitiveTopology::ePatchList,
    };
    return std::ranges::find(unsupported_topologies, topology) == unsupported_topologies.end();
}

auto IsLine(vk::PrimitiveTopology topology) -> bool {
    static constexpr std::array line_topologies{vk::PrimitiveTopology::eLineList,
                                                vk::PrimitiveTopology::eLineStrip};
    return std::ranges::find(line_topologies, topology) == line_topologies.end();
}

auto MsaaModeToVk(MsaaMode msaa_mode) -> vk::SampleCountFlagBits {
    switch (msaa_mode) {
        case MsaaMode::Msaa1x1:
            return vk::SampleCountFlagBits::e1;
        case MsaaMode::Msaa2x1:
        case MsaaMode::Msaa2x1_D3D:
            return vk::SampleCountFlagBits::e2;
        case MsaaMode::Msaa2x2:
        case MsaaMode::Msaa2x2_VC4:
        case MsaaMode::Msaa2x2_VC12:
            return vk::SampleCountFlagBits::e4;
        case MsaaMode::Msaa4x2:
        case MsaaMode::Msaa4x2_D3D:
        case MsaaMode::Msaa4x2_VC8:
        case MsaaMode::Msaa4x2_VC24:
            return vk::SampleCountFlagBits::e8;
        case MsaaMode::Msaa4x4:
            return vk::SampleCountFlagBits::e16;
        default:
            ASSERT(false &&
                   fmt::format("Invalid msaa_mode={}", static_cast<int>(msaa_mode)).c_str());
            return vk::SampleCountFlagBits::e1;
    }
}

auto MakeRenderPassKey(const FixedPipelineState& state) -> RenderPassKey {
    RenderPassKey key{};
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

}  // namespace

auto GraphicsPipelineCacheKey::operator==(const GraphicsPipelineCacheKey& rhs) const noexcept
    -> bool {
    return std::memcmp(&rhs, this, Size()) == 0;
}

auto GraphicsPipelineCacheKey::Hash() const noexcept -> size_t {
    const u64 hash = XXH64(reinterpret_cast<const char*>(this), Size(), 0);
    return static_cast<size_t>(hash);
}

GraphicsPipeline::GraphicsPipeline(
    scheduler::Scheduler& scheduler, VulkanPipelineCache& pipeline_cache_,
    ShaderNotify* shader_notify, const Device& device, resource::DescriptorPool& descriptor_pool,
    GuestDescriptorQueue& guest_descriptor_queue_, common::ThreadWorker* worker_thread,
    RenderPassCache& render_pass_cache, const GraphicsPipelineCacheKey& key,
    TextureCache& texture_cache_, BufferCache& buffer_cache_,
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
      buffer_cache(buffer_cache_),
      use_dynamic_render(settings::values.use_dynamic_rendering.GetValue()) {
    if (shader_notify) {
        shader_notify->MarkShaderBuilding();
    }
    uint32_t push_constant_size = 0;
    for (u32 stage = 0; stage < NUM_STAGES; ++stage) {
        const shader::Info* const info{gsl::at(infos, stage)};
        if (!info) {
            continue;
        }
        gsl::at(stage_infos, stage) = *info;
        num_textures += shader::NumDescriptors(info->texture_descriptors);
        if (info->push_constants.size > 0) {
            push_constant_size = info->push_constants.size;
        }
    }
    auto func{[this, shader_notify, &render_pass_cache, &descriptor_pool,
               push_constant_size] -> void {
        pipeline::DescriptorLayoutBuilder builder{MakeBuilder(device_, stage_infos)};
        uses_push_descriptor = builder.CanUsePushDescriptor();
        descriptor_set_layout = builder.CreateDescriptorSetLayout(uses_push_descriptor);
        if (!uses_push_descriptor) {
            descriptor_allocator = descriptor_pool.allocator(*descriptor_set_layout, stage_infos);
        }
        const vk::DescriptorSetLayout set_layout{*descriptor_set_layout};
        pipeline_layout = builder.CreatePipelineLayout(set_layout, push_constant_size);
        descriptor_update_template =
            builder.CreateTemplate(set_layout, *pipeline_layout, uses_push_descriptor);

        const vk::RenderPass render_pass =
            use_dynamic_render ? VK_NULL_HANDLE
                               : render_pass_cache.get(MakeRenderPassKey(key_.state));
        validate();
        makePipeline(render_pass);

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
}

void GraphicsPipeline::AddTransition(GraphicsPipeline* transition) {
    transition_keys.push_back(transition->key_);
    transitions.push_back(transition);
}

void GraphicsPipeline::Configure() {
    guest_descriptor_queue_.Acquire();
    buffer_cache.BindGraphicUniformBuffer();
    auto textures = texture_cache.getCurrentTextures();
    auto* sample = texture_cache.getSampler(SamplerPreset::Linear);
    for (const auto& texture : textures) {
        if (texture) {
            guest_descriptor_queue_.AddSampledImage(texture->RenderTarget(), sample->Handle());
        }
    }
    ConfigureDraw();
}

void GraphicsPipeline::ConfigureDraw() {
    if (use_dynamic_render) {
        scheduler_.requestRender(texture_cache.getFramebuffer()->getRenderingRequest());
    } else {
        scheduler_.requestRender(texture_cache.getFramebuffer()->getRenderPassRequest());
    }

    if (!is_built.load(std::memory_order::relaxed)) {
        // Wait for the pipeline to be built
        scheduler_.record([this](vk::CommandBuffer) {
            std::unique_lock lock{build_mutex};
            build_condvar.wait(lock, [this] { return is_built.load(std::memory_order::relaxed); });
        });
    }
    const bool bind_pipeline{scheduler_.updateGraphicsPipeline(*pipeline)};
    const void* const descriptor_data{guest_descriptor_queue_.UpdateData()};
    const auto push = buffer_cache.GetPushConstants();
    scheduler_.record([this, push, descriptor_data, bind_pipeline](vk::CommandBuffer cmdbuf) {
        if (bind_pipeline) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
        }
        if (!push.empty()) {
            cmdbuf.pushConstants(*pipeline_layout, vk::ShaderStageFlagBits::eAllGraphics, 0,
                                 push.size(), push.data());
        }

        if (!descriptor_set_layout) {
            return;
        }
        if (uses_push_descriptor) {
            cmdbuf.pushDescriptorSetWithTemplate(*descriptor_update_template, *pipeline_layout, 0,
                                                 descriptor_data);
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
    static_vector<vk::VertexInputBindingDescription, 32> vertex_bindings;
    static_vector<vk::VertexInputBindingDivisorDescriptionEXT, 32> vertex_binding_divisors;
    static_vector<vk::VertexInputAttributeDescription, 32> vertex_attributes;
    vk::PipelineVertexInputStateCreateInfo vertex_input_ci =
        vk::PipelineVertexInputStateCreateInfo()
            .setVertexBindingDescriptions(vertex_bindings)
            .setVertexAttributeDescriptions(vertex_attributes);
    const vk::PipelineVertexInputDivisorStateCreateInfoEXT input_divisor_ci =
        vk::PipelineVertexInputDivisorStateCreateInfoEXT().setVertexBindingDivisors(
            vertex_binding_divisors);

    if (!vertex_binding_divisors.empty()) {
        vertex_input_ci.setPNext(&input_divisor_ci);
    }

    const FixedPipelineState::DynamicState& pipeline_dynamic = key_.state.dynamicState;

    auto input_assembly_topology = PrimitiveTopologyToVK(key_.state.topology.Value());
    auto primitiveRestartEnable = (input_assembly_topology != vk::PrimitiveTopology::ePatchList &&
                                   device_.IsTopologyListPrimitiveRestartSupported()) ||
                                  (SupportsPrimitiveRestart(input_assembly_topology) ||
                                   (input_assembly_topology == vk::PrimitiveTopology::ePatchList &&
                                    device_.IsPatchListPrimitiveRestartSupported()));
    const vk::PipelineInputAssemblyStateCreateInfo input_assembly_ci =
        vk::PipelineInputAssemblyStateCreateInfo()
            .setTopology(input_assembly_topology)
            .setPrimitiveRestartEnable(primitiveRestartEnable);

    const vk::PipelineTessellationStateCreateInfo tessellation_ci =
        vk::PipelineTessellationStateCreateInfo().setPatchControlPoints(3);

    vk::PipelineViewportDepthClipControlCreateInfoEXT ndc_info =
        vk::PipelineViewportDepthClipControlCreateInfoEXT().setNegativeOneToOne(
            key_.state.ndc_minus_one_to_one.Value() != 0 ? VK_TRUE : VK_FALSE);

    const u32 num_viewports = std::min<u32>(device_.GetMaxViewports(), 16);

    vk::PipelineViewportStateCreateInfo viewport_ci = vk::PipelineViewportStateCreateInfo()
                                                          .setViewportCount(num_viewports)
                                                          .setScissorCount(num_viewports);

    if (device_.IsExtDepthClipControlSupported()) {
        ndc_info.pNext = std::exchange(viewport_ci.pNext, &ndc_info);
    }
    vk::PipelineRasterizationStateCreateInfo rasterization_ci =
        vk::PipelineRasterizationStateCreateInfo()
            .setRasterizerDiscardEnable(VK_TRUE)
            .setPolygonMode(vk::PolygonMode::eFill);
    vk::PipelineRasterizationLineStateCreateInfoEXT line_state =
        vk::PipelineRasterizationLineStateCreateInfoEXT()
            .setLineRasterizationMode(key_.state.smooth_lines.Value() != 0
                                          ? vk::LineRasterizationMode::eRectangularSmooth
                                          : vk::LineRasterizationMode::eRectangular)
            .setStippledLineEnable(pipeline_dynamic.line_stipple_enable ? VK_TRUE : VK_FALSE)
            .setLineStippleFactor(key_.state.line_stipple_factor)
            .setLineStipplePattern(static_cast<uint16_t>(key_.state.line_stipple_pattern));
    auto conservative_raster =
        vk::PipelineRasterizationConservativeStateCreateInfoEXT().setConservativeRasterizationMode(
            key_.state.conservative_raster_enable
                ? vk::ConservativeRasterizationModeEXT::eOverestimate
                : vk::ConservativeRasterizationModeEXT::eDisabled);
    vk::PipelineRasterizationProvokingVertexStateCreateInfoEXT provoking_vertex =
        vk::PipelineRasterizationProvokingVertexStateCreateInfoEXT().setProvokingVertexMode(
            key_.state.provoking_vertex_last ? vk::ProvokingVertexModeEXT::eFirstVertex
                                             : vk::ProvokingVertexModeEXT::eLastVertex);

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

    const auto multisample_ci = vk::PipelineMultisampleStateCreateInfo()
                                    .setRasterizationSamples(MsaaModeToVk(key_.state.msaa_mode))
                                    .setSampleShadingEnable(VK_FALSE)
                                    .setMinSampleShading(0.0f)
                                    .setAlphaToCoverageEnable(key_.state.alpha_to_coverage_enabled)
                                    .setAlphaToOneEnable(key_.state.alpha_to_one_enabled);
    const vk::PipelineDepthStencilStateCreateInfo depth_stencil_ci{};

    if (pipeline_dynamic.depth_bounds_enable && !device_.IsDepthBoundsSupported()) {
        SPDLOG_WARN("Depth bounds is enabled but not supported");
    }
    static_vector<vk::PipelineColorBlendAttachmentState, 8> cb_attachments;
    const size_t num_attachments{NumAttachments(key_.state)};
    for (size_t index = 0; index < num_attachments; ++index) {
        static constexpr std::array mask_table{
            vk::ColorComponentFlagBits::eR,
            vk::ColorComponentFlagBits::eG,
            vk::ColorComponentFlagBits::eB,
            vk::ColorComponentFlagBits::eA,

        };
        vk::ColorComponentFlags write_mask{};
        for (auto i : mask_table) {
            write_mask |= i;
        }
        cb_attachments.push_back(vk::PipelineColorBlendAttachmentState()
                                     .setBlendEnable(VK_TRUE)
                                     .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                                     .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                                     .setColorBlendOp(vk::BlendOp::eAdd)
                                     .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                                     .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                                     .setAlphaBlendOp(vk::BlendOp::eAdd)
                                     .setColorWriteMask(write_mask));
    }
    const auto color_blend_ci =
        vk::PipelineColorBlendStateCreateInfo().setAttachments(cb_attachments);
    static_vector<vk::DynamicState, 36> dynamic_states{
        vk::DynamicState::eViewport,         vk::DynamicState::eScissor,
        vk::DynamicState::eDepthBias,        vk::DynamicState::eBlendConstants,
        vk::DynamicState::eDepthBounds,      vk::DynamicState::eStencilCompareMask,
        vk::DynamicState::eStencilWriteMask, vk::DynamicState::eStencilReference,
        vk::DynamicState::eLineWidth};
    if (dynamic.has_extended_dynamic_state) {
        static constexpr std::array extended{
            vk::DynamicState::eCullMode,          vk::DynamicState::eFrontFace,
            vk::DynamicState::eDepthTestEnable,   vk::DynamicState::eDepthWriteEnable,
            vk::DynamicState::eDepthCompareOp,    vk::DynamicState::eDepthBoundsTestEnable,
            vk::DynamicState::eStencilTestEnable, vk::DynamicState::eStencilOp};
        if (key_.state.dynamic_vertex_input) {
            dynamic_states.push_back(vk::DynamicState::eVertexInputEXT);
            dynamic_states.push_back(vk::DynamicState::eVertexInputBindingStride);
        }
        dynamic_states.insert(dynamic_states.end(), extended.begin(), extended.end());

        if (dynamic.has_extended_dynamic_state_2) {
            static constexpr std::array extended2{
                vk::DynamicState::eDepthBiasEnable,
                vk::DynamicState::ePrimitiveRestartEnable,
                vk::DynamicState::eRasterizerDiscardEnable,
            };
            dynamic_states.insert(dynamic_states.end(), extended2.begin(), extended2.end());
        }

        if (dynamic.has_extended_dynamic_state_3_blend) {
            static constexpr std::array extended3{
                vk::DynamicState::eColorBlendEnableEXT,
                vk::DynamicState::eColorBlendEquationEXT,
                vk::DynamicState::eColorWriteMaskEXT,
            };
            dynamic_states.insert(dynamic_states.end(), extended3.begin(), extended3.end());
        }

        if (dynamic.has_extended_dynamic_state_3_enables) {
            static constexpr std::array extended3{
                vk::DynamicState::eDepthClampEnableEXT,
            };
            dynamic_states.insert(dynamic_states.end(), extended3.begin(), extended3.end());
        }
    }

    const auto dynamic_state_ci =
        vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamic_states);

    static_vector<vk::PipelineShaderStageCreateInfo, 5> shader_stages;
    for (uint32_t stage = 0; stage < 5; ++stage) {
        if (!spv_modules_[stage]) {
            continue;
        }
        [[maybe_unused]] auto& stage_ci =
            shader_stages.emplace_back(vk::PipelineShaderStageCreateInfo()
                                           .setStage(ShaderStage(stage))
                                           .setModule(*spv_modules_[stage])
                                           .setPName("main")

            );
    }
    vk::PipelineCreateFlags flags{};
    if (device_.IsKhrPipelineExecutablePropertiesEnabled()) {
        flags |= vk::PipelineCreateFlagBits::eCaptureStatisticsKHR;
    }
    small_vector<vk::Format, 8> formats;
    for (const auto& format : key_.state.color_formats) {
        if (format != surface::PixelFormat::Invalid) {
            auto vk_format = device_.surfaceFormat(FormatType::Optimal, true, format);
            formats.push_back(vk_format.format);
        }
    }
    auto rendering_ci = vk::PipelineRenderingCreateInfo().setColorAttachmentFormats(formats);
    if (key_.state.depth_format != surface::PixelFormat::Invalid) {
        auto vk_format = device_.surfaceFormat(FormatType::Optimal, true, key_.state.depth_format);
        rendering_ci.setDepthAttachmentFormat(vk_format.format);
    }

    const vk::GraphicsPipelineCreateInfo pipeline_ci =
        vk::GraphicsPipelineCreateInfo()
            .setFlags(flags)
            .setStages(shader_stages)
            .setPVertexInputState(&vertex_input_ci)
            .setPInputAssemblyState(&input_assembly_ci)
            .setPTessellationState(&tessellation_ci)
            .setPViewportState(&viewport_ci)
            .setPRasterizationState(&rasterization_ci)
            .setPMultisampleState(&multisample_ci)
            .setPDepthStencilState(&depth_stencil_ci)
            .setPColorBlendState(&color_blend_ci)
            .setPDynamicState(&dynamic_state_ci)
            .setLayout(*pipeline_layout)
            .setRenderPass(render_pass)
            .setPNext(use_dynamic_render ? &rendering_ci : nullptr);
    pipeline = device_.logical().createPipeline(pipeline_ci);
}
}  // namespace render::vulkan