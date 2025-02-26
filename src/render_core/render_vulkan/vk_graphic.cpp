#include "vk_graphic.hpp"
#include "uniforms.hpp"
#include "blit_screen.hpp"
#include <imgui_impl_vulkan.h>
#include <tracy/Tracy.hpp>
namespace render::vulkan {

VulkanGraphics::VulkanGraphics(core::frontend::BaseWindow* emu_window_, const Device& device_,
                               MemoryAllocator& memory_allocator_, scheduler::Scheduler& scheduler_,
                               ShaderNotify& shader_notify_, ImguiCore* imgui_)
    : device(device_),
      memory_allocator(memory_allocator_),
      scheduler(scheduler_),
      imgui(imgui_),
      staging_pool(device, memory_allocator, scheduler),
      descriptor_pool(device, scheduler),
      guest_descriptor_queue(device, scheduler),
      compute_pass_descriptor_queue(device, scheduler),
      blit_image(device, scheduler, descriptor_pool),
      render_pass_cache(device),
      emu_window(emu_window_),
      texture_cache_runtime{device,
                            scheduler,
                            memory_allocator,
                            staging_pool,
                            render_pass_cache,
                            descriptor_pool,
                            compute_pass_descriptor_queue,
                            blit_image},
      texture_cache(texture_cache_runtime),
      buffer_cache_runtime(device, memory_allocator, scheduler, staging_pool,
                           guest_descriptor_queue, compute_pass_descriptor_queue, descriptor_pool),
      buffer_cache(buffer_cache_runtime),
      pipeline_cache(device, scheduler, descriptor_pool, guest_descriptor_queue, render_pass_cache,
                     buffer_cache, texture_cache, shader_notify_),
      wfi_event(device.logical().createEvent()) {}

VulkanGraphics::~VulkanGraphics() = default;

void VulkanGraphics::start() {
    // TODO 这里是一个临时策略
    std::scoped_lock lock{texture_cache.mutex};
    static bool is_new_frame = true;
    if (is_new_frame) {
        is_new_frame = false;
        return;
    }
    scheduler.requestRenderPass(texture_cache.GetFramebuffer());
    clear();
}
void VulkanGraphics::setPipelineState(const PipelineState& state) { pipeline_state = state; }

#if defined(USE_DEBUG_UI)
auto VulkanGraphics::getDrawImage() -> ImTextureID {
    // 将 Vulkan 纹理绑定到 ImGui
    const auto& image_view = texture_cache.TryFindFramebufferImageView({});
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
                .setMipLodBias(0.0f)
                .setMinLod(0.0f)
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
#else
auto VulkanGraphics::getDrawImage() -> ImTextureID { return 0; }
#endif

void VulkanGraphics::UpdateDynamicStates() {
    UpdateViewportsState();
    UpdateScissorsState();
    UpdateDepthBias();
    UpdateBlendConstants();
    UpdateDepthBounds();
    UpdateStencilFaces();
    if (device.IsExtExtendedDynamicStateSupported()) {
        UpdateCullMode();
        UpdateDepthCompareOp();
        UpdateFrontFace();
        UpdateStencilOp();

        if (true) {  // TODO 这里临时用
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
                UpdateLogicOpEnable();
                UpdateDepthClampEnable();
            }
        }
        if (device.IsExtExtendedDynamicState2ExtrasSupported()) {
            UpdateLogicOp();
        }
        if (device.IsExtExtendedDynamicState3Supported()) {
            UpdateBlending();
        }
    }

    if (device.IsExtVertexInputDynamicStateSupported()) {
        UpdateVertexInput();
    }
}
// 用于启用或禁用 图元重启（Primitive
// Restart）功能。图元重启功能主要用于在绘制图元（如三角形、线条等）时，
// 允许在索引缓冲区中插入一个特殊值（称为“重启索引”），以分隔不同的图元序列。
void VulkanGraphics::UpdatePrimitiveRestartEnable() {
    scheduler.record([this,
                      enable = pipeline_state.primitiveRestartEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setPrimitiveRestartEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

// 用于启用或禁用 光栅化丢弃
void VulkanGraphics::UpdateRasterizerDiscardEnable() {
    scheduler.record([this,
                      enable = pipeline_state.rasterizerDiscardEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setRasterizerDiscardEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateDepthBiasEnable() {
    const bool enable = pipeline_state.depthBiasEnable;
    scheduler.record([enable, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthBiasEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateVertexInput() {
    boost::container::static_vector<vk::VertexInputBindingDescription2EXT, 32> bindings;
    boost::container::static_vector<vk::VertexInputAttributeDescription2EXT, 32> attributes;

    attributes.push_back(vk::VertexInputAttributeDescription2EXT()
                             .setBinding(0)
                             .setLocation(0)
                             .setFormat(vk::Format::eR32G32B32Sfloat)
                             .setOffset(offsetof(Vertex, position)));
    attributes.push_back(vk::VertexInputAttributeDescription2EXT()
                             .setBinding(0)
                             .setLocation(1)
                             .setFormat(vk::Format::eR32G32B32Sfloat)
                             .setOffset(offsetof(Vertex, color)));
    attributes.push_back(vk::VertexInputAttributeDescription2EXT()
                             .setBinding(0)
                             .setLocation(2)
                             .setFormat(vk::Format::eR32G32Sfloat)
                             .setOffset(offsetof(Vertex, texCoord)));
    bindings.push_back(vk::VertexInputBindingDescription2EXT()
                           .setBinding(0)
                           .setStride(sizeof(Vertex))
                           .setInputRate(vk::VertexInputRate::eVertex)
                           .setDivisor(1));

    scheduler.record([this, bindings, attributes](vk::CommandBuffer cmdbuf) {
        cmdbuf.setVertexInputEXT(bindings, attributes, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateCullMode() {
    scheduler.record([enabled = pipeline_state.cullMode, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setCullModeEXT(enabled ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone,
                              device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateDepthCompareOp() {
    scheduler.record([this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthCompareOpEXT(vk::CompareOp::eLessOrEqual,
                                    device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateFrontFace() {
    scheduler.record([this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setFrontFaceEXT(vk::FrontFace::eCounterClockwise,
                               device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateStencilOp() {
    // Front face defines the stencil op of both faces
    scheduler.record([this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setStencilOpEXT(vk::StencilFaceFlagBits::eFront, vk::StencilOp::eReplace,
                               vk::StencilOp::eReplace, vk::StencilOp::eReplace,
                               vk::CompareOp::eAlways, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateDepthBoundsTestEnable() {
    bool enabled = pipeline_state.depthBoundsTestEnable;
    if (enabled && !device.IsDepthBoundsSupported()) {
        SPDLOG_WARN("Depth bounds is enabled but not supported");
        enabled = false;
    }
    scheduler.record([enable = enabled, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthBoundsTestEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateDepthTestEnable() {
    scheduler.record([enable = pipeline_state.depthTestEnable, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthTestEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}
/**
 * @brief 设置这个后深度信息显示正常了
 *
 */
void VulkanGraphics::UpdateDepthWriteEnable() {
    scheduler.record([enable = pipeline_state.depthWriteEnable, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthWriteEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateStencilTestEnable() {
    scheduler.record([enable = pipeline_state.stencilTestEnable, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setStencilTestEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateLogicOpEnable() {
    scheduler.record([enable = pipeline_state.logicOpEnable, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setLogicOpEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateDepthClampEnable() {
    scheduler.record(
        [is_enabled = pipeline_state.depthClampEnable, this](vk::CommandBuffer cmdbuf) {
            cmdbuf.setDepthClampEnableEXT(is_enabled, device.logical().getDispatchLoaderDynamic());
        });
}

void VulkanGraphics::UpdateLogicOp() {
    scheduler.record([this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setLogicOpEXT(vk::LogicOp::eAnd, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateBlending() {
    std::array<vk::ColorComponentFlags, 1> setup_masks{};
    for (auto& current : setup_masks) {
        current |= vk::ColorComponentFlagBits::eR;
        current |= vk::ColorComponentFlagBits::eG;
        current |= vk::ColorComponentFlagBits::eB;
        current |= vk::ColorComponentFlagBits::eA;
    }
    scheduler.record([this, setup_masks](vk::CommandBuffer cmdbuf) {
        cmdbuf.setColorWriteMaskEXT(0, setup_masks, device.logical().getDispatchLoaderDynamic());
    });
    std::array<VkBool32, 1> setup_enables{VK_FALSE};
    if (pipeline_state.colorBlendEnable) {
        setup_enables[0] = VK_TRUE;
    }
    scheduler.record([this, setup_enables](vk::CommandBuffer cmdbuf) {
        cmdbuf.setColorBlendEnableEXT(0, setup_enables,
                                      device.logical().getDispatchLoaderDynamic());
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
    scheduler.record([this, setup_blends](vk::CommandBuffer cmdbuf) {
        cmdbuf.setColorBlendEquationEXT(0, setup_blends,
                                        device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateViewportsState() {
    auto view =
        vk::Viewport()
            .setX(pipeline_state.viewport.x)
            .setY(pipeline_state.viewport.y)
            .setWidth(pipeline_state.viewport.width == 0.f ? 1.f : pipeline_state.viewport.width)
            .setHeight(pipeline_state.viewport.height == 0.f ? 1.f : pipeline_state.viewport.height)
            .setMinDepth(pipeline_state.viewport.minDepth)
            .setMaxDepth(pipeline_state.viewport.maxDepth);
    scheduler.record([view](vk::CommandBuffer cmdbuf) { cmdbuf.setViewport(0, view); });
    return;
}

void VulkanGraphics::UpdateScissorsState() {
    vk::Rect2D scissor;
    scissor.offset.x = pipeline_state.scissors.x;
    scissor.offset.y = pipeline_state.scissors.y;
    scissor.extent.width =
        static_cast<u32>(pipeline_state.scissors.width != 0 ? pipeline_state.scissors.width : 1);
    scissor.extent.height =
        static_cast<u32>(pipeline_state.scissors.height != 0 ? pipeline_state.scissors.height : 1);
    scheduler.record([scissor](vk::CommandBuffer cmdbuf) { cmdbuf.setScissor(0, scissor); });
}

auto VulkanGraphics::AccelerateDisplay(const frame::FramebufferConfig& config,
                                       u32 pixel_stride) -> std::optional<FramebufferTextureInfo> {
    std::scoped_lock lock{texture_cache.mutex};
    const auto& image_view = texture_cache.TryFindFramebufferImageView(config);
    if (!image_view.first) {
        return std::nullopt;
    }
    FramebufferTextureInfo info{};
    info.image = image_view.first->ImageHandle();
    info.image_view = image_view.first->Handle(shader::TextureType::Color2D);
    info.width = image_view.first->size.width;
    info.height = image_view.first->size.height;
    info.scaled_width = info.width;
    info.scaled_height = info.height;
    return info;
}

void VulkanGraphics::UpdateDepthBias() {
    scheduler.record([](vk::CommandBuffer cmdbuf) { cmdbuf.setDepthBias(0.0f, .0f, .0f); });
}

void VulkanGraphics::UpdateBlendConstants() {
    const std::array blend_color = {pipeline_state.blendColor.r, pipeline_state.blendColor.g,
                                    pipeline_state.blendColor.b, pipeline_state.blendColor.a};
    scheduler.record(
        [blend_color](vk::CommandBuffer cmdbuf) { cmdbuf.setBlendConstants(blend_color.data()); });
}

void VulkanGraphics::UpdateDepthBounds() {
    scheduler.record([](vk::CommandBuffer cmdbuf) { cmdbuf.setDepthBounds(0, 1); });
}

void VulkanGraphics::UpdateStencilFaces() {
    scheduler.record([](vk::CommandBuffer cmdbuf) {
        // Front face
        cmdbuf.setStencilReference(vk::StencilFaceFlagBits::eBack, 1);
    });
    scheduler.record([](vk::CommandBuffer cmdbuf) {
        // Front face
        cmdbuf.setStencilWriteMask(vk::StencilFaceFlagBits::eBack, 1);
    });

    scheduler.record([](vk::CommandBuffer cmdbuf) {
        // Front face
        cmdbuf.setStencilCompareMask(vk::StencilFaceFlagBits::eBack, 1);
    });
}

void VulkanGraphics::UpdateLineWidth() {
    scheduler.record([](vk::CommandBuffer cmdbuf) { cmdbuf.setLineWidth(1); });
}
void VulkanGraphics::clear() {
    const vk::Extent2D render_area{static_cast<uint32_t>(pipeline_state.viewport.width),
                                   static_cast<uint32_t>(pipeline_state.viewport.height)};
    const f32 bg_red = pipeline_state.clearColor.r;
    const f32 bg_green = pipeline_state.clearColor.g;
    const f32 bg_blue = pipeline_state.clearColor.b;
    auto clear_value = vk::ClearValue().setColor(
        vk::ClearColorValue().setFloat32({bg_red, bg_green, bg_blue, 1.0f}));
    const vk::ClearAttachment clear_attachment = vk::ClearAttachment()
                                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                     .setColorAttachment(0)
                                                     .setClearValue(clear_value);

    auto clear_depth = vk::ClearDepthStencilValue()
                           .setDepth(pipeline_state.clearColor.depth)
                           .setStencil(pipeline_state.clearColor.stencil);
    auto clear_depth_value = vk::ClearValue().setDepthStencil(clear_depth);
    const vk::ClearAttachment depth_attachment = vk::ClearAttachment()
                                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                     .setColorAttachment(0)
                                                     .setClearValue(clear_depth_value);

    const vk::ClearRect clear_rect =
        vk::ClearRect()
            .setRect(vk::Rect2D().setOffset(vk::Offset2D().setX(0).setY(0)).setExtent(render_area))
            .setBaseArrayLayer(0)
            .setLayerCount(1);
    scheduler.record([clear_attachment, depth_attachment, clear_rect](vk::CommandBuffer cmdbuf) {
        cmdbuf.clearAttachments({clear_attachment, depth_attachment}, {clear_rect});
    });
}

void VulkanGraphics::drawImgui(vk::CommandBuffer cmd_buf) {
#if defined(USE_DEBUG_UI)
    imgui->draw(cmd_buf);
#endif
}
auto VulkanGraphics::addGraphicContext(const GraphicsContext& context) -> GraphicsId {
    auto [viewId, samplerId] = texture_cache.addGraphics(context.image);
    RenderTargetInfo info{};
    info.vertex_size = static_cast<u32>(context.vertex.size() * sizeof(float));
    info.indices_size = context.indices_size;
    info.image_view_id = viewId;
    info.sampler_id = samplerId;
    info.vertex_buffer_id = buffer_cache.addVertexBuffer(context.vertex.data(), info.vertex_size);

    info.indices_buffer_id = buffer_cache.addIndexBuffer(
        context.indices.data(), static_cast<u32>(context.indices.size() * sizeof(uint16_t)));
    info.uniform_buffer_size = context.uniform_size;
    info.uniform_buffer_id = buffer_cache.addUniformBuffer(info.uniform_buffer_size);
    auto graphicsId = slot_graphics.insert();
    draw_indices[graphicsId] = info;
    return graphicsId;
}
void VulkanGraphics::bindUniformBuffer(GraphicsId id, void* data, size_t size) {
    auto draw_info = draw_indices[id];
    buffer_cache.BindUniformBuffers(draw_info.uniform_buffer_id, data, size);
}
void VulkanGraphics::draw(GraphicsId id) {
    const auto drawInfo = draw_indices[id];
    buffer_cache.setCurrentUniformBuffer(drawInfo.uniform_buffer_id, drawInfo.uniform_buffer_size);
    texture_cache.setCurrentImage(drawInfo.image_view_id, drawInfo.sampler_id);
    PrepareDraw(true, [drawInfo, this] {
        buffer_cache.BindVertexBuffers(drawInfo.vertex_buffer_id, drawInfo.vertex_size);
        buffer_cache.BindIndexBuffer(drawInfo.indices_buffer_id);
        scheduler.record([indices_size = drawInfo.indices_size](vk::CommandBuffer cmdbuf) {
            cmdbuf.drawIndexed(indices_size, 1, 0, 0, 0);
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
}

template <typename Func>
void VulkanGraphics::PrepareDraw(bool is_indexed, Func&& draw_func) {
    ZoneScopedN("VulkanGraphics::PrepareDraw()");
    GraphicsPipeline* const pipeline{pipeline_cache.currentGraphicsPipeline()};
    if (!pipeline) {
        return;
    }
    if (!pipeline) {
        return;
    }
    std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
    pipeline->Configure(is_indexed);
    UpdateDynamicStates();
    draw_func();
    FlushWork();
}

void VulkanGraphics::FlushWork() {
    static constexpr u32 DRAWS_TO_DISPATCH = 4096;

    // Only check multiples of 8 draws
    static_assert(DRAWS_TO_DISPATCH % 8 == 0);
    if ((++draw_counter & 7) != 7) {
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
}

}  // namespace render::vulkan