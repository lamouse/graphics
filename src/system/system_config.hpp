#pragma once
#include <vulkan/vulkan.hpp>
namespace config {
struct ImageQuality {
        ::vk::SampleCountFlagBits msaaSamples;
};

auto getDeviceExtensions() -> ::std::vector<const char *>;

}  // namespace config