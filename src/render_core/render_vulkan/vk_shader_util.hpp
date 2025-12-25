#pragma once
#include<vulkan/vulkan.hpp>
import render.vulkan.common;
namespace render::vulkan::utils {
auto buildShader(vk::Device device, std::span<const uint32_t> code) -> ShaderModule;
}