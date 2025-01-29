#include "vk_graphic.hpp"
#include "uniforms.hpp"
#include "blit_screen.hpp"
namespace render::vulkan {
VulkanGraphics::VulkanGraphics(core::frontend::BaseWindow* emu_window_, const Device& device_,
                               MemoryAllocator& memory_allocator_, scheduler::Scheduler& scheduler_)
    : device(device_),
      memory_allocator(memory_allocator_),
      scheduler(scheduler_),
      staging_pool(device, memory_allocator, scheduler),
      descriptor_pool(device, scheduler),
      guest_descriptor_queue(device, scheduler),
      compute_pass_descriptor_queue(device, scheduler),
      blit_image(device, scheduler, descriptor_pool),
      render_pass_cache(device),
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
                     buffer_cache, texture_cache),
      wfi_event(device.logical().createEvent()) {}

VulkanGraphics::~VulkanGraphics() = default;

void VulkanGraphics::addTexture(const texture::ImageInfo& imageInfo) {
    image_view_id = texture_cache.CreateImageView(imageInfo);
    sampler_id = texture_cache.CreateSampler(image_view_id);
}

void VulkanGraphics::addVertex(std::span<float> vertex, const ::std::span<uint16_t>& indices) {
    buffer_cache.BindVertexBuffers(vertex.data(), static_cast<u32>(vertex.size() * sizeof(float)));
    buffer_cache.BindIndexBuffer(indices.data(),
                                 static_cast<u32>(indices.size() * sizeof(uint16_t)));
}

void VulkanGraphics::addUniformBuffer(void* data, size_t size) {
    uniform_buffer_id = buffer_cache.BindUniforBuffers(0, 0, data, static_cast<u32>(size));
}

void VulkanGraphics::drawIndics(u32 indicesSize) {
    guest_descriptor_queue.AddSampledImage(
        texture_cache.GetImageView(image_view_id).Handle(shader::TextureType::Color2D),
        texture_cache.GetSampler(sampler_id).Handle());
    auto* pipeline = pipeline_cache.currentGraphicsPipeline();
    // guest_descriptor_queue.TickFrame();
    pipeline->Configure(true);
    UpdateDynamicStates();
    scheduler.record([this, indicesSize](vk::CommandBuffer cmdbuf) {
        cmdbuf.setRasterizerDiscardEnableEXT(true, device.logical().getDispatchLoaderDynamic());
        cmdbuf.drawIndexed(indicesSize, 1, 0, 0, 0);
    });
}

void VulkanGraphics::UpdateDynamicStates() {
    UpdateViewportsState();
    UpdateScissorsState();
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

void VulkanGraphics::UpdatePrimitiveRestartEnable() {
    scheduler.record([this, enable = true](vk::CommandBuffer cmdbuf) {
        cmdbuf.setPrimitiveRestartEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateRasterizerDiscardEnable() {
    scheduler.record([this, disable = true](vk::CommandBuffer cmdbuf) {
        cmdbuf.setRasterizerDiscardEnableEXT(disable == 0,
                                             device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateDepthBiasEnable() {
    const u32 enable = true;
    scheduler.record([this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthBiasEnableEXT(enable != 0, device.logical().getDispatchLoaderDynamic());
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
    scheduler.record([enabled = true, this](vk::CommandBuffer cmdbuf) {
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
    if (1) {
        // Separate stencil op per face

        // scheduler.Record([this](vk::CommandBuffer cmdbuf) {
        //     cmdbuf.setStencilOpEXT(VK_STENCIL_FACE_FRONT_BIT, MaxwellToVK::StencilOp(fail),
        //                            MaxwellToVK::StencilOp(zpass), MaxwellToVK::StencilOp(zfail),
        //                            MaxwellToVK::ComparisonOp(compare));
        //     cmdbuf.SetStencilOpEXT(VK_STENCIL_FACE_BACK_BIT, MaxwellToVK::StencilOp(back_fail),
        //                            MaxwellToVK::StencilOp(back_zpass),
        //                            MaxwellToVK::StencilOp(back_zfail),
        //                            MaxwellToVK::ComparisonOp(back_compare));
        // });
    } else {
        // Front face defines the stencil op of both faces
        scheduler.record([this](vk::CommandBuffer cmdbuf) {
            cmdbuf.setStencilOpEXT(vk::StencilFaceFlagBits::eVkStencilFrontAndBack,
                                   vk::StencilOp::eReplace, vk::StencilOp::eReplace,
                                   vk::StencilOp::eReplace, vk::CompareOp::eAlways,
                                   device.logical().getDispatchLoaderDynamic());
        });
    }
}

void VulkanGraphics::UpdateDepthBoundsTestEnable() {
    bool enabled = true;
    if (enabled && !device.IsDepthBoundsSupported()) {
        SPDLOG_WARN("Depth bounds is enabled but not supported");
        enabled = false;
    }
    scheduler.record([enable = enabled, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthBoundsTestEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateDepthTestEnable() {
    scheduler.record([enable = true, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthTestEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateDepthWriteEnable() {
    scheduler.record([enable = true, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthWriteEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateStencilTestEnable() {
    scheduler.record([enable = true, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setStencilTestEnableEXT(enable, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateLogicOpEnable() {
    scheduler.record([enable = 1, this](vk::CommandBuffer cmdbuf) {
        cmdbuf.setLogicOpEnableEXT(enable != 0, device.logical().getDispatchLoaderDynamic());
    });
}

void VulkanGraphics::UpdateDepthClampEnable() {
    scheduler.record([is_enabled = true, this](vk::CommandBuffer cmdbuf) {
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
    scheduler.record([this, setup_enables](vk::CommandBuffer cmdbuf) {
        cmdbuf.setColorBlendEnableEXT(0, setup_enables,
                                      device.logical().getDispatchLoaderDynamic());
    });

    std::array<vk::ColorBlendEquationEXT, 1> setup_blends{};
    for (size_t index = 0; index < setup_blends.size(); index++) {
        auto& host_blend = setup_blends[index];
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
    if (1) {  // TODO 这里需要修改
        auto view = vk::Viewport()
                        .setX(0)
                        .setY(0)
                        .setWidth(800)
                        .setHeight(600)
                        .setMinDepth(.0f)
                        .setMaxDepth(1.f);
        scheduler.record([view](vk::CommandBuffer cmdbuf) { cmdbuf.setViewport(0, view); });
        return;
    }
    // const bool is_rescaling{texture_cache.IsRescaling()};
    // const float scale = is_rescaling ? Settings::values.resolution_info.up_factor : 1.0f;
    // const std::array viewport_list{
    //     GetViewportState(device, regs, 0, scale),  GetViewportState(device, regs, 1, scale),
    //     GetViewportState(device, regs, 2, scale),  GetViewportState(device, regs, 3, scale),
    //     GetViewportState(device, regs, 4, scale),  GetViewportState(device, regs, 5, scale),
    //     GetViewportState(device, regs, 6, scale),  GetViewportState(device, regs, 7, scale),
    //     GetViewportState(device, regs, 8, scale),  GetViewportState(device, regs, 9, scale),
    //     GetViewportState(device, regs, 10, scale), GetViewportState(device, regs, 11, scale),
    //     GetViewportState(device, regs, 12, scale), GetViewportState(device, regs, 13, scale),
    //     GetViewportState(device, regs, 14, scale), GetViewportState(device, regs, 15, scale),
    // };
    // scheduler.record([this, viewport_list](vk::CommandBuffer cmdbuf) {
    //     const u32 num_viewports = std::min<u32>(device.GetMaxViewports(), Maxwell::NumViewports);
    //     const vk::Span<VkViewport> viewports(viewport_list.data(), num_viewports);
    //     cmdbuf.SetViewport(0, viewports);
    // });
}

void VulkanGraphics::UpdateScissorsState() {
    if (1) {  // TODO 这里需要修改
        const auto x = 0;
        const auto y = 0;
        const auto width = 800;
        const auto height = 600;
        vk::Rect2D scissor;
        scissor.offset.x = static_cast<u32>(x);
        scissor.offset.y = static_cast<u32>(y);
        scissor.extent.width = static_cast<u32>(width != 0.0f ? width : 1.0f);
        scissor.extent.height = static_cast<u32>(height != 0.0f ? height : 1.0f);
        scheduler.record([scissor](vk::CommandBuffer cmdbuf) { cmdbuf.setScissor(0, scissor); });
        return;
    }
    // u32 up_scale = 1;
    // u32 down_shift = 0;
    // if (texture_cache.IsRescaling()) {
    //     up_scale = Settings::values.resolution_info.up_scale;
    //     down_shift = Settings::values.resolution_info.down_shift;
    // }
    // const std::array scissor_list{
    //     GetScissorState(regs, 0, up_scale, down_shift),
    //     GetScissorState(regs, 1, up_scale, down_shift),
    //     GetScissorState(regs, 2, up_scale, down_shift),
    //     GetScissorState(regs, 3, up_scale, down_shift),
    //     GetScissorState(regs, 4, up_scale, down_shift),
    //     GetScissorState(regs, 5, up_scale, down_shift),
    //     GetScissorState(regs, 6, up_scale, down_shift),
    //     GetScissorState(regs, 7, up_scale, down_shift),
    //     GetScissorState(regs, 8, up_scale, down_shift),
    //     GetScissorState(regs, 9, up_scale, down_shift),
    //     GetScissorState(regs, 10, up_scale, down_shift),
    //     GetScissorState(regs, 11, up_scale, down_shift),
    //     GetScissorState(regs, 12, up_scale, down_shift),
    //     GetScissorState(regs, 13, up_scale, down_shift),
    //     GetScissorState(regs, 14, up_scale, down_shift),
    //     GetScissorState(regs, 15, up_scale, down_shift),
    // };
    // scheduler.Record([this, scissor_list](vk::CommandBuffer cmdbuf) {
    //     const u32 num_scissors = std::min<u32>(device.GetMaxViewports(), Maxwell::NumViewports);
    //     const vk::Span<VkRect2D> scissors(scissor_list.data(), num_scissors);
    //     cmdbuf.SetScissor(0, scissors);
    // });
}

auto VulkanGraphics::AccelerateDisplay(const frame::FramebufferConfig& config,
                                       u32 pixel_stride) -> std::optional<FramebufferTextureInfo> {
    std::scoped_lock lock{texture_cache.mutex};
    const auto& image_view = texture_cache.GetImageView(image_view_id);

    FramebufferTextureInfo info{};
    info.image = image_view.ImageHandle();
    info.image_view = image_view.Handle(shader::TextureType::Color2D);
    info.width = image_view.size.width;
    info.height = image_view.size.height;
    info.scaled_width = info.width;
    info.scaled_height = info.height;
    return info;
}

}  // namespace render::vulkan