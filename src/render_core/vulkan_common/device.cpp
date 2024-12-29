#include "device.hpp"
#include <spdlog/spdlog.h>
namespace render::vulkan {
Device::Device(vk::Instance instance, vk::PhysicalDevice physical, vk::SurfaceKHR surface)
    : instance_(instance), physical_(physical) {
    format_properties_ = utils::GetFormatProperties(physical_);
}
auto Device::GetSuitability(bool requires_swapchain) -> bool {
    // Assume we will be suitable.
    bool suitable = true;

    return suitable;
}
}  // namespace render::vulkan
