#include "blit_screen.hpp"
#include "present/filters.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"
#include "vulkan_common/device.hpp"
#include "vulkan_common/memory_allocator.hpp"
#include "scheduler.hpp"
#include "present_manager.hpp"
#include "present/layer.hpp"
#include "present/window_adapt_pass.hpp"
#include "common/settings.hpp"

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

}  // namespace render::vulkan