#include "render_vulkan.hpp"
namespace render::vulkan {
auto createDevice(const Instance& instance, vk::SurfaceKHR surface) -> Device {
    auto physical = instance.EnumeratePhysicalDevices();
    return Device(*instance, physical[0], surface);
}
}  // namespace render::vulkan