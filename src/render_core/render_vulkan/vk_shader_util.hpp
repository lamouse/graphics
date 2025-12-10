#pragma once
#include <vulkan/vulkan.hpp>
#include "vulkan_common/vulkan_wrapper.hpp"
namespace render::vulkan::utils {
auto buildShader(vk::Device device, std::span<const uint32_t> code) -> ShaderModule;
}