#include "blit_screen.hpp"
#include <spdlog/spdlog.h>
#include "present/filters.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"
#include "vulkan_common/device.hpp"
#include "vulkan_common/memory_allocator.hpp"
#include "scheduler.hpp"
#include "present_manager.hpp"
#include "present/layer.hpp"
#include "present/window_adapt_pass.hpp"
#include "common/settings.hpp"

#include "render_core/render_vulkan/vk_graphic.hpp"
namespace render::vulkan {
BlitScreen::BlitScreen(const Device& device_, MemoryAllocator& memory_allocator_,
                       PresentManager& present_manager_, scheduler::Scheduler& scheduler_)
    : device{device_},
      memory_allocator{memory_allocator_},
      present_manager{present_manager_},
      scheduler{scheduler_},
      image_count{1},
      swapchain_view_format{vk::Format::eB8G8R8A8Unorm} {}
BlitScreen::~BlitScreen() = default;

void BlitScreen::WaitIdle() {
    present_manager.waitPresent();
    scheduler.finish();
    device.getLogical().waitIdle();
}

void BlitScreen::SetWindowAdaptPass() {
    layers.clear();
    scaling_filter = common::settings::get<settings::Graphics>().scaling_filter;

    switch (scaling_filter) {
        case settings::enums::ScalingFilter::NearestNeighbor:
            window_adapt = MakeNearestNeighbor(device, swapchain_view_format);
            break;
        case settings::enums::ScalingFilter::Bicubic:
            window_adapt = MakeBicubic(device, swapchain_view_format);
            break;
        case settings::enums::ScalingFilter::Gaussian:
            window_adapt = MakeGaussian(device, swapchain_view_format);
            break;
        case settings::enums::ScalingFilter::ScaleForce:
            window_adapt = MakeScaleForce(device, swapchain_view_format);
            break;
        case settings::enums::ScalingFilter::Fsr:
        case settings::enums::ScalingFilter::Bilinear:
        default:
            window_adapt = MakeBilinear(device, swapchain_view_format);
            break;
    }
}

auto BlitScreen::CreateFramebuffer(const vk::ImageView& image_view, vk::Extent2D extent,
                                   vk::RenderPass render_pass) -> VulkanFramebuffer {
    return device.logical().createFramerBuffer(
        vk::FramebufferCreateInfo{{}, render_pass, image_view, extent.width, extent.height, 1});
}

[[nodiscard]] auto BlitScreen::CreateFramebuffer(vk::ImageView image_view,
                                                 const layout::FrameBufferLayout& layout,
                                                 vk::Format current_view_format)
    -> VulkanFramebuffer {
    const bool format_updated =
        std::exchange(swapchain_view_format, current_view_format) != current_view_format;
    if (!window_adapt ||
        scaling_filter != common::settings::get<settings::Graphics>().scaling_filter ||
        format_updated) {
        WaitIdle();
        SetWindowAdaptPass();
    }
    const vk::Extent2D extent{
        layout.width,
        layout.height,
    };
    return CreateFramebuffer(image_view, extent, window_adapt->getRenderPass());
}

void BlitScreen::DrawToFrame(VulkanGraphics& rasterizer, Frame* frame,
                             const layout::FrameBufferLayout& layout,
                             std::span<const frame::FramebufferConfig> framebuffers,
                             size_t current_swapchain_image_count,
                             vk::Format current_swapchain_view_format) {
    bool resource_update_required = false;
    bool presentation_recreate_required = false;

    // Recreate dynamic resources if the adapting filter changed
    if (!window_adapt ||
        scaling_filter != common::settings::get<settings::Graphics>().scaling_filter) {
        resource_update_required = true;
    }

    // Recreate dynamic resources if the image count changed
    const size_t old_swapchain_image_count =
        std::exchange(image_count, current_swapchain_image_count);
    if (old_swapchain_image_count != current_swapchain_image_count) {
        resource_update_required = true;
    }

    // Recreate the presentation frame if the format or dimensions of the window changed
    const vk::Format old_swapchain_view_format =
        std::exchange(swapchain_view_format, current_swapchain_view_format);
    if (old_swapchain_view_format != current_swapchain_view_format ||
        layout.width != frame->width || layout.height != frame->height) {
        resource_update_required = true;
        presentation_recreate_required = true;
    }

    // If we have a pending resource update, perform it
    if (resource_update_required) {
        // Wait for idle to ensure no resources are in use
        WaitIdle();

        // Update window adapt pass
        SetWindowAdaptPass();

        // Update frame format if needed
        if (presentation_recreate_required) {
            present_manager.recreateFrame(frame, layout.width, layout.height, swapchain_view_format,
                                          window_adapt->getRenderPass());
        }
    }

    // Add additional layers if needed
    const vk::Extent2D window_size{
        layout.width,
        layout.height,
    };

    while (layers.size() < framebuffers.size()) {
        layers.emplace_back(device, memory_allocator, scheduler, image_count, window_size,
                            window_adapt->getDescriptorSetLayout());
    }

    // Perform the draw
    window_adapt->Draw(rasterizer, scheduler, image_index, layers, framebuffers, layout, frame);

    // Advance to next image
    if (++image_index >= image_count) {
        image_index = 0;
    }
}

}  // namespace render::vulkan