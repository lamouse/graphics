#pragma once
#include <vulkan/vulkan.hpp>
namespace config {
struct ImageQuality {
        ::vk::SampleCountFlagBits msaaSamples;
};

}  // namespace config