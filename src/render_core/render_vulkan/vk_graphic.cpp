#include "vk_graphic.hpp"
#include "blit_screen.hpp"
#include "render_core/render_vulkan/format_to_vk.hpp"
#include <imgui_impl_vulkan.h>
#include <algorithm>
#include <tracy/Tracy.hpp>

namespace {
auto buildVertexAttribute(const render::vulkan::Device& device,
                          std::span<render::VertexAttribute> attributes)
    -> boost::container::static_vector<vk::VertexInputAttributeDescription2EXT, 32> {
    boost::container::static_vector<vk::VertexInputAttributeDescription2EXT, 32> attrs;
    for (size_t i = 0; i < attributes.size(); i++) {
        auto attribute = attributes[i];
        attrs.push_back(
            vk::VertexInputAttributeDescription2EXT()
                .setBinding(attribute.binding)
                .setLocation(i)
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
                                      .setInputRate(vk::VertexInputRate::eVertex)
                                      .setDivisor(1));
    }

    return vertex_bindings;
}
}  // namespace

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
      wfi_event(device.logical().createEvent()) {}

VulkanGraphics::~VulkanGraphics() = default;

void VulkanGraphics::clean() {
    scheduler.requestRenderPass(texture_cache.getFramebuffer());
    clear();
}
void VulkanGraphics::clear() {
    auto config = emu_window->getActiveConfig();
    const vk::Extent2D render_area{ static_cast<std::uint32_t>(config.extent.width), static_cast<std::uint32_t>(config.extent.height)};
    const f32 bg_red = pipeline_state.clearColor.r;
    const f32 bg_green = pipeline_state.clearColor.g;
    const f32 bg_blue = pipeline_state.clearColor.b;
    auto clear_value = vk::ClearValue().setColor(
        vk::ClearColorValue().setFloat32({bg_red, bg_green, bg_blue, 1.0F}));
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
void VulkanGraphics::setPipelineState(const PipelineState& state) { pipeline_state = state; }

#if defined(USE_DEBUG_UI)
auto VulkanGraphics::getDrawImage() -> ImTextureID {
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
    return 0;
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
    auto resource = modelResource[current_modelId];
    auto attrs = vertex_attributes[resource.vertex_attribute_id];
    auto bindings = vertex_bindings[resource.vertex_binding_id];
    scheduler.record([this, bindings, attrs](vk::CommandBuffer cmdbuf) -> void {
        cmdbuf.setVertexInputEXT(bindings, attrs, device.logical().getDispatchLoaderDynamic());
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

auto VulkanGraphics::AccelerateDisplay(const frame::FramebufferConfig& config, u32 pixel_stride)
    -> std::optional<FramebufferTextureInfo> {
    std::scoped_lock lock{texture_cache.mutex};
    const auto& image_view = texture_cache.TryFindFramebufferImageView();

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

void VulkanGraphics::drawImgui(vk::CommandBuffer cmd_buf) {
#if defined(USE_DEBUG_UI)
    imgui->draw(cmd_buf);
#endif
}

auto VulkanGraphics::uploadModel(const graphics::IModelInstance& instance) -> ModelId {
    ModelResource resource;
    if (auto imageData = instance.getImageData()) {
        auto viewId = texture_cache.addTexture({.width = static_cast<u32>(imageData->getWidth()),
                                                .height = static_cast<u32>(imageData->getheight())},
                                               imageData->data());
        resource.image_view = viewId;
    }
    auto meshData = instance.getMeshData();
    resource.vertex_size = static_cast<u32>(meshData->getMesh().size() * sizeof(float));
    resource.vertex_buffer_id =
        buffer_cache.addVertexBuffer(meshData->getMesh().data(), resource.vertex_size);
    resource.indices_buffer_id =
        buffer_cache.addIndexBuffer(meshData->getIndices().data(), meshData->getIndices().size());
    resource.indices_count = meshData->getIndicesSize();

    auto vertexAttribute = meshData->getVertexAttribute();
    resource.vertex_attribute_id =
        vertex_attributes.insert(buildVertexAttribute(device, std::span(vertexAttribute)));
    auto vertexBinding = meshData->getVertexBinding();
    resource.vertex_binding_id =
        vertex_bindings.insert(buildVertexBinding(std::span(vertexBinding)));
    return modelResource.insert(resource);
}

void VulkanGraphics::draw(const graphics::IModelInstance& instance) {
    current_modelId = instance.getModelId();
    const auto resource = modelResource[current_modelId];
    texture_cache.setCurrentTexture(resource.image_view, SamplerPreset::Linear);
    guest_descriptor_queue.Acquire();
    buffer_cache.BindUniformBuffer(instance.getUBOData());
    auto [view, sample] = texture_cache.getCurrentTexture();
    guest_descriptor_queue.AddSampledImage(view->Handle(shader::TextureType::Color2D),
                                           sample->Handle());
    texture::FramebufferKey key;
    key.size.width = emu_window->getActiveConfig().extent.width;
    key.size.height = emu_window->getActiveConfig().extent.height;
    key.size.depth = 1;
    std::ranges::fill(key.color_formats, render::surface::PixelFormat::Invalid);
    key.color_formats.at(0) = surface::PixelFormat::B8G8R8A8_UNORM;
    key.depth_format = surface::PixelFormat::D32_FLOAT;
    texture_cache.setCurrentFrameBuffer(key);
    PrepareDraw(true, [resource, this] -> void {
        IndexFormat index_format{IndexFormat::UnsignedShort};
        if (resource.indices_count > std::numeric_limits<uint16_t>::max()) {
            index_format = IndexFormat::UnsignedInt;
        }
        buffer_cache.BindVertexBuffers(resource.vertex_buffer_id, resource.vertex_size);
        buffer_cache.BindIndexBuffer(index_format, resource.indices_buffer_id);
        scheduler.record([indices_size = resource.indices_count](vk::CommandBuffer cmdbuf) {
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
}

}  // namespace render::vulkan