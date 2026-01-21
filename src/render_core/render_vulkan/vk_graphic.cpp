module;
#include <boost/container/static_vector.hpp>
#include <tracy/Tracy.hpp>
#include <span>
#include <vulkan/vulkan.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <ktx.h>

#include "imgui_impl_vulkan.h"
#include "common/assert.hpp"

#ifdef MemoryBarrier
#undef MemoryBarrier
#endif
module render.vulkan;
import render.vulkan.format_utils;
import render.vulkan.present.present_frame;
import render.vulkan.scheduler;
import render.vulkan.pipeline_cache;
import render.vulkan.graphics_pipeline;
import render.shader_cache;
import render.shader_notify;
import render.framebuffer_config;
import render.buffer.cache;
import render.graphic;
import render.texture.texture_cache;
import render;
import shader;
import core;
import common;
using IsInstance = bool;

namespace {
auto buildVertexAttribute(const render::vulkan::Device& device,
                          std::span<render::VertexAttribute> attributes)
    -> boost::container::static_vector<vk::VertexInputAttributeDescription2EXT, 32> {
    boost::container::static_vector<vk::VertexInputAttributeDescription2EXT, 32> attrs;
    for (const auto& attribute : attributes) {
        attrs.push_back(
            vk::VertexInputAttributeDescription2EXT()
                .setBinding(0)
                .setLocation(attribute.location)
                .setFormat(render::vulkan::VertexFormat(device, attribute.type, attribute.size))
                .setOffset(attribute.offset));
    }

    return attrs;
}

auto buildVertexBinding(std::span<render::VertexBinding> bindings)
    -> boost::container::static_vector<vk::VertexInputBindingDescription2EXT, 32> {
    boost::container::static_vector<vk::VertexInputBindingDescription2EXT, 32> vertex_bindings;
    for (auto binding : bindings) {
        vertex_bindings.push_back(vk::VertexInputBindingDescription2EXT()
                                      .setBinding(binding.binding)
                                      .setStride(binding.stride)
                                      .setInputRate(binding.is_instance
                                                        ? vk::VertexInputRate::eInstance
                                                        : vk::VertexInputRate::eVertex)
                                      .setDivisor(binding.divisor));
    }

    return vertex_bindings;
}
}  // namespace

namespace render::vulkan {

VulkanGraphics::VulkanGraphics(core::frontend::BaseWindow* emu_window_, const Device& device_,
                               MemoryAllocator& memory_allocator_, scheduler::Scheduler& scheduler_,
                               ShaderNotify& shader_notify_)
    : device(device_),
      memory_allocator(memory_allocator_),
      scheduler(scheduler_),
      staging_pool(device, memory_allocator, scheduler),
      descriptor_pool(device, scheduler.getMasterSemaphore()),
      guest_descriptor_queue(device, scheduler),
      compute_pass_descriptor_queue(device, scheduler),
      render_pass_cache(device),
      emu_window(emu_window_),
      texture_cache_runtime{device,
                            scheduler,
                            memory_allocator,
                            staging_pool,
                            render_pass_cache,
                            descriptor_pool,
                            compute_pass_descriptor_queue},
      texture_cache(texture_cache_runtime),
      buffer_cache_runtime(device, memory_allocator, scheduler, staging_pool,
                           guest_descriptor_queue, compute_pass_descriptor_queue),
      buffer_cache(buffer_cache_runtime),
      pipeline_cache(device, scheduler, descriptor_pool, guest_descriptor_queue, render_pass_cache,
                     buffer_cache, texture_cache, shader_notify_),
      wfi_event(device.logical().createEvent()),
      use_dynamic_render(settings::values.use_dynamic_rendering.GetValue()) {}

VulkanGraphics::~VulkanGraphics() = default;

void VulkanGraphics::clean(const CleanValue& cleanValue) {
    std::scoped_lock lock{texture_cache.mutex};
    texture::FramebufferKey key;
    key.size = cleanValue.framebuffer.extent;
    key.depth_format = cleanValue.framebuffer.depth_format;
    key.color_formats = cleanValue.framebuffer.color_formats;
    texture_cache.UpdateRenderTarget(key);
    auto* framebuffer = texture_cache.getFramebuffer();
    if (!framebuffer->HasAspectColorBit(0) && !framebuffer->HasAspectDepthBit() &&
        !framebuffer->HasAspectStencilBit()) {
        return;
    }
    auto frame_render_area = framebuffer->RenderArea();
    auto num_render_pass_images_ = framebuffer->NumImages();
    const auto& render_pass_images_ = framebuffer->Images();
    const auto& render_pass_image_ranges_ = framebuffer->ImageRanges();
    if (use_dynamic_render) {
        scheduler.requestRender(framebuffer->getRenderingRequest());
    } else {
        scheduler.requestRender(framebuffer->getRenderPassRequest());
    }

    const vk::Extent2D render_area{cleanValue.width, cleanValue.hight};
    const vk::ClearRect clear_rect =
        vk::ClearRect()
            .setRect(
                vk::Rect2D()
                    .setOffset(vk::Offset2D().setX(cleanValue.offset_x).setY(cleanValue.offset_y))
                    .setExtent(render_area))
            .setBaseArrayLayer(0)
            .setLayerCount(1);

    if (framebuffer->HasAspectColorBit(0)) {
        auto clear_value =
            vk::ClearValue().setColor(vk::ClearColorValue().setFloat32(cleanValue.clear_color));
        const vk::ClearAttachment clear_attachment =
            vk::ClearAttachment()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setColorAttachment(0)
                .setClearValue(clear_value);
        scheduler.record([clear_attachment, clear_rect](vk::CommandBuffer cmdbuf) {
            cmdbuf.clearAttachments(clear_attachment, {clear_rect});
        });
    }

    if (framebuffer->HasAspectDepthBit()) {
        auto clear_depth =
            vk::ClearDepthStencilValue().setDepth(cleanValue.clear_depth).setStencil(0);
        auto clear_depth_value = vk::ClearValue().setDepthStencil(clear_depth);
        const vk::ClearAttachment depth_attachment =
            vk::ClearAttachment()
                .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                .setColorAttachment(0)
                .setClearValue(clear_depth_value);

        scheduler.record([depth_attachment, clear_rect](vk::CommandBuffer cmdbuf) {
            cmdbuf.clearAttachments(depth_attachment, {clear_rect});
        });
    }
}

void VulkanGraphics::dispatchCompute(const IComputeInstance& instance) {
    pipeline_cache.setCurrentShader(instance.getShaderHash());
    FlushWork();
    auto work = instance.getWorkgroupSize();

    auto* pipeline{pipeline_cache.currentComputePipeline(work)};
    if (!pipeline) {
        return;
    }
    buffer_cache.UploadComputeUniformBuffer(instance.getUBOData());
    for (const auto& mesh : instance.getMeshIds()) {
        const auto resource = modelResource[mesh];
        buffer_cache.bindComputeStorageBuffers(resource.vertex_buffer_id);
    }

    std::scoped_lock lock{texture_cache.mutex, buffer_cache.mutex};
    pipeline->Configure(scheduler, buffer_cache);

    static constexpr vk::MemoryBarrier READ_BARRIER =
        vk::MemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite)
            .setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
    scheduler.requestOutsideRenderOperationContext();
    scheduler.record([](vk::CommandBuffer cmdbuf) {
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlagBits::eComputeShader, {}, READ_BARRIER, nullptr,
                               nullptr);
    });
    scheduler.record(
        [work](vk::CommandBuffer cmdbuf) -> void { cmdbuf.dispatch(work[0], work[1], work[2]); });
}

auto VulkanGraphics::getDrawImage() -> unsigned long long {
    // 将 Vulkan 纹理绑定到 ImGui
    const auto& image_view = texture_cache.TryFindFramebufferImageView();
    if (!sampler) {
        ::vk::SamplerCreateInfo samplerInfo =
            ::vk::SamplerCreateInfo()
                .setMagFilter(::vk::Filter::eLinear)
                .setMinFilter(::vk::Filter::eLinear)
                .setAddressModeU(::vk::SamplerAddressMode::eRepeat)
                .setAddressModeV(::vk::SamplerAddressMode::eRepeat)
                .setAddressModeW(::vk::SamplerAddressMode::eRepeat)
                .setAnisotropyEnable(VK_TRUE)
                .setMaxAnisotropy(device.getMaxAnisotropy())
                .setBorderColor(::vk::BorderColor::eIntOpaqueBlack)
                .setUnnormalizedCoordinates(VK_FALSE)
                .setCompareEnable(VK_FALSE)
                .setCompareOp(::vk::CompareOp::eAlways)
                .setMipmapMode(::vk::SamplerMipmapMode::eLinear)
                .setMipLodBias(0.0F)
                .setMinLod(0.0F)
                .setMaxLod(1);
        sampler = device.logical().CreateSampler(samplerInfo);
    }
    const auto [pair, is_new] =
        imgui_textures.try_emplace(image_view.first->Handle(shader::TextureType::Color2D));
    if (!is_new) {
        return pair->second;
    }
    ImTextureID imguiTextureID_{};

    imguiTextureID_ = (ImTextureID)ImGui_ImplVulkan_AddTexture(
        *sampler, image_view.first->Handle(shader::TextureType::Color2D), VK_IMAGE_LAYOUT_GENERAL);
    pair->second = imguiTextureID_;
    return imguiTextureID_;
}

void VulkanGraphics::UpdateDynamicStates() {
    UpdateViewportsState();
    UpdateScissorsState();
    UpdateDepthBias();
    UpdateBlendConstants();
    UpdateDepthBounds();
    UpdateStencilFaces();
    UpdateLineWidth();

    if (device.IsExtExtendedDynamicStateSupported()) {
        UpdateCullMode();
        UpdateDepthCompareOp();
        UpdateFrontFace();
        UpdateStencilOp();

        UpdateDepthBoundsTestEnable();
        UpdateDepthTestEnable();
        UpdateDepthWriteEnable();
        UpdateStencilTestEnable();
        if (device.IsExtExtendedDynamicState2Supported()) {
            UpdatePrimitiveRestartEnable();
            UpdateRasterizerDiscardEnable();
            UpdateDepthBiasEnable();
        }
        if (device.IsExtExtendedDynamicState3EnablesSupported()) {
            UpdateDepthClampEnable();
            UpdateBlending();
        }
    }

    if (device.IsExtVertexInputDynamicStateSupported()) {
        auto* pipeline = pipeline_cache.currentGraphicsPipeline(current_primitive_topology);
        if (pipeline && pipeline->HasDynamicVertexInput()) {
            UpdateVertexInput();
        }
    }
}
// 用于启用或禁用 图元重启（Primitive
// Restart）功能。图元重启功能主要用于在绘制图元（如三角形、线条等）时，
// 允许在索引缓冲区中插入一个特殊值（称为“重启索引”），以分隔不同的图元序列。
void VulkanGraphics::UpdatePrimitiveRestartEnable() {
    if (!is_begin_frame && last_pipeline_state.primitiveRestartEnable ==
                               current_pipeline_state.primitiveRestartEnable) {
        return;
    }

    if (!is_begin_frame) {
        last_pipeline_state.primitiveRestartEnable = current_pipeline_state.primitiveRestartEnable;
    }

    scheduler.record(
        [enable = last_pipeline_state.primitiveRestartEnable](vk::CommandBuffer cmdbuf) {
            cmdbuf.setPrimitiveRestartEnableEXT(enable);
        });
}

// 用于启用或禁用 光栅化丢弃
void VulkanGraphics::UpdateRasterizerDiscardEnable() {
    if (!is_begin_frame && last_pipeline_state.rasterizerDiscardEnable ==
                               current_pipeline_state.rasterizerDiscardEnable) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.rasterizerDiscardEnable =
            current_pipeline_state.rasterizerDiscardEnable;
    }
    scheduler.record([enable = last_pipeline_state.rasterizerDiscardEnable](
                         vk::CommandBuffer cmdbuf) { cmdbuf.setRasterizerDiscardEnable(enable); });
}

void VulkanGraphics::UpdateDepthBiasEnable() {
    if (!is_begin_frame &&
        last_pipeline_state.depthBiasEnable == current_pipeline_state.depthBiasEnable) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.depthBiasEnable = current_pipeline_state.depthBiasEnable;
    }
    const bool enable = last_pipeline_state.depthBiasEnable;
    scheduler.record([enable](vk::CommandBuffer cmdbuf) { cmdbuf.setDepthBiasEnable(enable); });
}

void VulkanGraphics::UpdateVertexInput() {
    if (current_modelId) {
        auto resource = modelResource[current_modelId];
        auto attrs = vertex_attributes[resource.vertex_attribute_id];
        auto bindings = vertex_bindings[resource.vertex_binding_id];
        scheduler.record([bindings, attrs](vk::CommandBuffer cmdbuf) -> void {
            cmdbuf.setVertexInputEXT(bindings, attrs);
        });
    } else {
        scheduler.record([](vk::CommandBuffer cmdbuf) -> void {
            cmdbuf.setVertexInputEXT(0, nullptr, 0, nullptr);
        });
    }
}

void VulkanGraphics::UpdateCullMode() {
    if (!is_begin_frame && last_pipeline_state.cullMode == current_pipeline_state.cullMode &&
        last_pipeline_state.cullFace == current_pipeline_state.cullFace) {
        return;
    }

    if (!is_begin_frame) {
        last_pipeline_state.cullMode = current_pipeline_state.cullMode;
        last_pipeline_state.cullFace = current_pipeline_state.cullFace;
    }
    scheduler.record([enabled = last_pipeline_state.cullMode,
                      mode = last_pipeline_state.cullFace](vk::CommandBuffer cmdbuf) {
        cmdbuf.setCullMode(enabled ? CullFace(mode) : vk::CullModeFlagBits::eNone);
    });
}

void VulkanGraphics::UpdateDepthCompareOp() {
    if (!is_begin_frame &&
        last_pipeline_state.depthComparison == current_pipeline_state.depthComparison) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.depthComparison = current_pipeline_state.depthComparison;
    }
    scheduler.record([op = last_pipeline_state.depthComparison](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthCompareOp(ComparisonOp(static_cast<render::ComparisonOp>(op)));
    });
}

void VulkanGraphics::UpdateFrontFace() {
    if (!is_begin_frame) {
        return;
    }
    scheduler.record(
        [](vk::CommandBuffer cmdbuf) { cmdbuf.setFrontFace(vk::FrontFace::eCounterClockwise); });
}

void VulkanGraphics::UpdateStencilOp() {
    if (!is_begin_frame &&
        last_pipeline_state.stencil_two_side_enable ==
            current_pipeline_state.stencil_two_side_enable &&
        last_pipeline_state.frontStencilOp == current_pipeline_state.frontStencilOp &&
        last_pipeline_state.backStencilOp == current_pipeline_state.backStencilOp) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.stencil_two_side_enable =
            current_pipeline_state.stencil_two_side_enable;
        last_pipeline_state.frontStencilOp = current_pipeline_state.frontStencilOp;
        last_pipeline_state.backStencilOp = current_pipeline_state.backStencilOp;
    }

    auto front_fail = StencilOp(last_pipeline_state.frontStencilOp.fail);
    auto front_pass = StencilOp(last_pipeline_state.frontStencilOp.pass);
    auto front_depth_fail = StencilOp(last_pipeline_state.frontStencilOp.depthFail);
    auto front_compare = ComparisonOp(last_pipeline_state.frontStencilOp.compare);
    if (last_pipeline_state.stencil_two_side_enable) {
        auto back_fail = StencilOp(last_pipeline_state.backStencilOp.fail);
        auto back_pass = StencilOp(last_pipeline_state.backStencilOp.pass);
        auto back_depth_fail = StencilOp(last_pipeline_state.backStencilOp.depthFail);
        auto back_compare = ComparisonOp(last_pipeline_state.backStencilOp.compare);
        scheduler.record([front_fail, front_pass, front_depth_fail, front_compare, back_fail,
                          back_pass, back_depth_fail, back_compare](vk::CommandBuffer cmdbuf) {
            cmdbuf.setStencilOp(vk::StencilFaceFlagBits::eFront, front_fail, front_pass,
                                front_depth_fail, front_compare);

            cmdbuf.setStencilOp(vk::StencilFaceFlagBits::eBack, back_fail, back_pass,
                                back_depth_fail, back_compare);
        });

    } else {
        scheduler.record(
            [front_fail, front_pass, front_depth_fail, front_compare](vk::CommandBuffer cmdbuf) {
                cmdbuf.setStencilOp(vk::StencilFaceFlagBits::eFrontAndBack, front_fail, front_pass,
                                    front_depth_fail, front_compare);
            });
    }
}

void VulkanGraphics::UpdateDepthBoundsTestEnable() {
    if (!is_begin_frame &&
        last_pipeline_state.depthBoundsTestEnable == current_pipeline_state.depthBoundsTestEnable) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.depthBoundsTestEnable = current_pipeline_state.depthBoundsTestEnable;
    }
    bool enabled = last_pipeline_state.depthBoundsTestEnable;
    if (enabled && !device.IsDepthBoundsSupported()) {
        SPDLOG_WARN("Depth bounds is enabled but not supported");
        enabled = false;
    }
    scheduler.record([enable = enabled](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthBoundsTestEnableEXT(enable);
    });
}

void VulkanGraphics::UpdateDepthTestEnable() {
    if (!is_begin_frame &&
        last_pipeline_state.depthTestEnable == current_pipeline_state.depthTestEnable) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.depthTestEnable = current_pipeline_state.depthTestEnable;
    }
    scheduler.record([enable = last_pipeline_state.depthTestEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthTestEnableEXT(enable);
    });
}

void VulkanGraphics::UpdateDepthWriteEnable() {
    if (!is_begin_frame &&
        last_pipeline_state.depthWriteEnable == current_pipeline_state.depthWriteEnable) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.depthWriteEnable = current_pipeline_state.depthWriteEnable;
    }
    scheduler.record([enable = last_pipeline_state.depthWriteEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthWriteEnableEXT(enable);
    });
}

void VulkanGraphics::UpdateStencilTestEnable() {
    if (!is_begin_frame &&
        last_pipeline_state.stencilTestEnable == current_pipeline_state.stencilTestEnable) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.stencilTestEnable = current_pipeline_state.stencilTestEnable;
    }
    scheduler.record([enable = last_pipeline_state.stencilTestEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setStencilTestEnable(enable);
    });
}

void VulkanGraphics::UpdateDepthClampEnable() {
    if (!is_begin_frame &&
        last_pipeline_state.depthClampEnable == current_pipeline_state.depthClampEnable) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.depthClampEnable = current_pipeline_state.depthClampEnable;
    }
    const auto enable = last_pipeline_state.depthClampEnable;
    scheduler.record([enable](vk::CommandBuffer cmdbuf) { cmdbuf.setDepthClampEnableEXT(enable); });
}

void VulkanGraphics::UpdateBlending() {
    if (!is_begin_frame &&
        last_pipeline_state.colorBlendEnable == current_pipeline_state.colorBlendEnable) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.colorBlendEnable = current_pipeline_state.colorBlendEnable;
    }
    std::array<vk::ColorComponentFlags, 1> setup_masks{};
    for (auto& current : setup_masks) {
        current |= vk::ColorComponentFlagBits::eR;
        current |= vk::ColorComponentFlagBits::eG;
        current |= vk::ColorComponentFlagBits::eB;
        current |= vk::ColorComponentFlagBits::eA;
    }
    scheduler.record(
        [setup_masks](vk::CommandBuffer cmdbuf) { cmdbuf.setColorWriteMaskEXT(0, setup_masks); });
    std::array<VkBool32, 1> setup_enables{VK_FALSE};
    if (last_pipeline_state.colorBlendEnable) {
        setup_enables[0] = VK_TRUE;
    }
    scheduler.record([setup_enables](vk::CommandBuffer cmdbuf) {
        cmdbuf.setColorBlendEnableEXT(0, setup_enables);
    });

    std::array<vk::ColorBlendEquationEXT, 1> setup_blends{};
    for (auto& host_blend : setup_blends) {
        host_blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        host_blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        host_blend.colorBlendOp = vk::BlendOp::eAdd;
        host_blend.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
        host_blend.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        host_blend.alphaBlendOp = vk::BlendOp::eAdd;
    }
    scheduler.record([setup_blends](vk::CommandBuffer cmdbuf) {
        cmdbuf.setColorBlendEquationEXT(0, setup_blends);
    });
}

void VulkanGraphics::UpdateViewportsState() {
    if (!is_begin_frame && last_pipeline_state.viewport == current_pipeline_state.viewport) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.viewport = current_pipeline_state.viewport;
    }
    auto view = vk::Viewport()
                    .setX(last_pipeline_state.viewport.x)
                    .setY(last_pipeline_state.viewport.y)
                    .setWidth(last_pipeline_state.viewport.width == 0.f
                                  ? 1.f
                                  : last_pipeline_state.viewport.width)
                    .setHeight(last_pipeline_state.viewport.height == 0.f
                                   ? 1.f
                                   : last_pipeline_state.viewport.height)
                    .setMinDepth(last_pipeline_state.viewport.minDepth)
                    .setMaxDepth(last_pipeline_state.viewport.maxDepth);
    scheduler.record([view](vk::CommandBuffer cmdbuf) { cmdbuf.setViewport(0, view); });
    return;
}

void VulkanGraphics::UpdateScissorsState() {
    if (!is_begin_frame && last_pipeline_state.scissors == current_pipeline_state.scissors) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.scissors = current_pipeline_state.scissors;
    }
    vk::Rect2D scissor;
    scissor.offset.x = last_pipeline_state.scissors.x;
    scissor.offset.y = last_pipeline_state.scissors.y;
    scissor.extent.width = static_cast<u32>(
        last_pipeline_state.scissors.width != 0 ? last_pipeline_state.scissors.width : 1);
    scissor.extent.height = static_cast<u32>(
        last_pipeline_state.scissors.height != 0 ? last_pipeline_state.scissors.height : 1);
    scheduler.record([scissor](vk::CommandBuffer cmdbuf) { cmdbuf.setScissor(0, scissor); });
}

auto VulkanGraphics::AccelerateDisplay(const frame::FramebufferConfig& config, u32 pixel_stride)
    -> std::optional<FramebufferTextureInfo> {
    std::scoped_lock lock{texture_cache.mutex};
    const auto& image_view = texture_cache.TryFindFramebufferImageView();

    FramebufferTextureInfo info{};
    info.image = image_view.first->ImageHandle();
    info.image_view = image_view.first->RenderTarget();
    info.width = image_view.first->size.width;
    info.height = image_view.first->size.height;
    info.scaled_width = info.width;
    info.scaled_height = info.height;
    return info;
}

void VulkanGraphics::UpdateDepthBias() {
    if (!is_begin_frame) {
        return;
    }
    scheduler.record([](vk::CommandBuffer cmdbuf) { cmdbuf.setDepthBias(0.0f, .0f, .0f); });
}

void VulkanGraphics::UpdateBlendConstants() {
    if (!is_begin_frame && last_pipeline_state.blendColor == current_pipeline_state.blendColor) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.blendColor = current_pipeline_state.blendColor;
    }
    const std::array blend_color = {
        last_pipeline_state.blendColor.r, last_pipeline_state.blendColor.g,
        last_pipeline_state.blendColor.b, last_pipeline_state.blendColor.a};
    scheduler.record(
        [blend_color](vk::CommandBuffer cmdbuf) { cmdbuf.setBlendConstants(blend_color.data()); });
}

void VulkanGraphics::UpdateDepthBounds() {
    if (!is_begin_frame) {
        return;
    }
    scheduler.record([](vk::CommandBuffer cmdbuf) { cmdbuf.setDepthBounds(0, 1); });
}

void VulkanGraphics::UpdateStencilFaces() {
    if (!is_begin_frame &&
        last_pipeline_state.stencil_two_side_enable ==
            current_pipeline_state.stencil_two_side_enable &&
        last_pipeline_state.stencilFrontProperties ==
            current_pipeline_state.stencilFrontProperties &&
        last_pipeline_state.stencilBackProperties == current_pipeline_state.stencilBackProperties) {
        return;
    }
    if (!is_begin_frame) {
        last_pipeline_state.stencil_two_side_enable =
            current_pipeline_state.stencil_two_side_enable;
        last_pipeline_state.stencilFrontProperties = current_pipeline_state.stencilFrontProperties;
        last_pipeline_state.stencilBackProperties = current_pipeline_state.stencilBackProperties;
    }
    scheduler.record(
        [two_sided = last_pipeline_state.stencil_two_side_enable,
         front_ref = last_pipeline_state.stencilFrontProperties.ref,
         back_ref = last_pipeline_state.stencilBackProperties.ref](vk::CommandBuffer cmdbuf) {
            const bool set_back = two_sided && front_ref != back_ref;

            // Front face
            cmdbuf.setStencilReference(
                set_back ? vk::StencilFaceFlagBits::eFront : vk::StencilFaceFlagBits::eFrontAndBack,
                front_ref);
            if (set_back) {
                cmdbuf.setStencilReference(vk::StencilFaceFlagBits::eBack, back_ref);
            }
        });

    scheduler.record([two_sided = last_pipeline_state.stencil_two_side_enable,
                      front_mask = last_pipeline_state.stencilFrontProperties.write_mask,
                      back_mask = last_pipeline_state.stencilBackProperties.write_mask](
                         vk::CommandBuffer cmdbuf) {
        const bool set_back = two_sided && front_mask != back_mask;

        // Front face
        cmdbuf.setStencilWriteMask(
            set_back ? vk::StencilFaceFlagBits::eFront : vk::StencilFaceFlagBits::eFrontAndBack,
            front_mask);
        if (set_back) {
            cmdbuf.setStencilWriteMask(vk::StencilFaceFlagBits::eBack, back_mask);
        }
    });

    scheduler.record([two_sided = last_pipeline_state.stencil_two_side_enable,
                      front_mask = last_pipeline_state.stencilFrontProperties.compare_mask,
                      back_mask = last_pipeline_state.stencilBackProperties.compare_mask](
                         vk::CommandBuffer cmdbuf) {
        const bool set_back = two_sided && front_mask != back_mask;

        // Front face
        cmdbuf.setStencilCompareMask(
            set_back ? vk::StencilFaceFlagBits::eFront : vk::StencilFaceFlagBits::eFrontAndBack,
            front_mask);
        if (set_back) {
            cmdbuf.setStencilCompareMask(vk::StencilFaceFlagBits::eBack, back_mask);
        }
    });
}

void VulkanGraphics::UpdateLineWidth() {
    if (!is_begin_frame) {
        return;
    }
    scheduler.record([](vk::CommandBuffer cmdbuf) { cmdbuf.setLineWidth(1); });
}

auto VulkanGraphics::uploadModel(const IMeshData& meshData) -> MeshId {
    ModelResource resource;
    auto mesh_data = meshData.getMesh();
    resource.vertex_size = static_cast<u32>(mesh_data.size() * sizeof(float));
    ASSERT_MSG(!mesh_data.empty(), "upload empty vertex index");
    resource.vertex_buffer_id =
        buffer_cache.addVertexBuffer(mesh_data.data(), resource.vertex_size);
    resource.vertex_count = meshData.getVertexCount();
    if (!meshData.getIndices().empty()) {
        resource.indices_buffer_id =
            buffer_cache.addIndexBuffer(meshData.getIndices().data(), meshData.getIndices().size());
        resource.indices_count = meshData.getIndicesSize();
    }

    auto vertexAttribute = meshData.getVertexAttribute();
    resource.vertex_attribute_id =
        vertex_attributes.insert(buildVertexAttribute(device, std::span(vertexAttribute)));
    auto vertexBinding = meshData.getVertexBinding();
    resource.vertex_binding_id =
        vertex_bindings.insert(buildVertexBinding(std::span(vertexBinding)));
    return modelResource.insert(resource);
}

auto VulkanGraphics::uploadTexture(const ITexture& texture) -> TextureId {
    return texture_cache.addTexture({.width = static_cast<u32>(texture.getWidth()),
                                     .height = static_cast<u32>(texture.getHeight())},
                                    texture.data(), texture.count());
}

auto VulkanGraphics::uploadTexture(ktxTexture* ktxTexture) -> TextureId {
    return texture_cache.addTexture(ktxTexture);
}

void VulkanGraphics::draw(const IMeshInstance& instance) {
    ZoneScopedN("VulkanGraphics::draw()");
    if (is_begin_frame) {
        last_pipeline_state = instance.getPipelineState();
    } else {
        current_pipeline_state = instance.getPipelineState();
    }

    pipeline_cache.setCurrentShader(instance.vertexShaderHash(), instance.fragmentShaderHash());
    current_modelId = instance.getMeshId();
    texture_cache.setCurrentTextures(instance.getMaterialIds());
    if (!instance.getUBOs().empty()) {
        buffer_cache.UploadGraphicUniformBuffer(instance.getUBOs());
    }
    if (!instance.getPushConstants().empty()) {
        buffer_cache.UploadPushConstants(instance.getPushConstants());
    }

    current_primitive_topology = instance.getPrimitiveTopology();
    int instance_vertex_count = 0;
    if (instance.getVertexCount() > 0) {
        instance_vertex_count = instance.getVertexCount();
    }

    auto render_command = instance.getRenderCommand();
    PrepareDraw([instance_vertex_count, render_command, this] -> void {
        uint32_t vertexCount = instance_vertex_count;

        if (current_modelId) {
            const auto resource = modelResource[current_modelId];
            auto bindings = vertex_bindings[resource.vertex_binding_id];
            buffer_cache.BindVertexBuffers(resource.vertex_buffer_id, resource.vertex_size,
                                           bindings[0].stride);
            if (resource.indices_buffer_id) {
                buffer_cache.BindIndexBuffer(IndexFormat::UnsignedInt, resource.indices_buffer_id);
            }
            vertexCount = resource.vertex_count;
        }

        scheduler.record([render_command, vertexCount](vk::CommandBuffer cmdbuf) -> void {
            if (render_command.indexCount > 0) {
                cmdbuf.drawIndexed(render_command.indexCount, 1, render_command.indexOffset, 0, 0);
            } else {
                cmdbuf.draw(vertexCount, 1, 0, 0);
            }
        });
    });
}

void VulkanGraphics::TickFrame() {
    guest_descriptor_queue.TickFrame();
    staging_pool.TickFrame();
    {
        std::scoped_lock lock{texture_cache.mutex};
        texture_cache.TickFrame();
    }
    {
        std::scoped_lock lock{buffer_cache.mutex};
        buffer_cache.TickFrame();
    }
    is_begin_frame = true;
}

template <typename Func>
void VulkanGraphics::PrepareDraw(Func&& draw_func) {
    ZoneScopedN("VulkanGraphics::PrepareDraw()");
    GraphicsPipeline* const pipeline{
        pipeline_cache.currentGraphicsPipeline(current_primitive_topology)};
    if (!pipeline) {
        return;
    }
    std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
    pipeline->Configure();
    UpdateDynamicStates();
    if (is_begin_frame) {
        is_begin_frame = false;
    }
    draw_func();
    FlushWork();
}

void VulkanGraphics::FlushWork() {
    static constexpr u32 DRAWS_TO_DISPATCH = 4096;

    // Only check multiples of 8 draws
    static_assert(DRAWS_TO_DISPATCH % 8 == 0);
    if ((++draw_counter & 7U) != 7) {
        return;
    }
    if (draw_counter < DRAWS_TO_DISPATCH) {
        // Send recorded tasks to the worker thread
        scheduler.dispatchWork();
        return;
    }
    // Otherwise (every certain number of draws) flush execution.
    // This submits commands to the Vulkan driver.
    scheduler.flush();
    draw_counter = 0;
    is_begin_frame = true;
}

}  // namespace render::vulkan