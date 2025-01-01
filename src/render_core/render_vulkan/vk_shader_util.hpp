#pragma once
#include <vulkan/vulkan.hpp>
namespace render::vulkan::utils {
auto buildShader(vk::Device device, std::span<const uint32_t> code) -> vk::ShaderModule;
}