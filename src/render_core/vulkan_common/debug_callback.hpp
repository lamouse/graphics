#pragma once
#include "vulkan_wrapper.hpp"
namespace render::vulkan {
[[nodiscard]] auto createDebugMessenger(::vk::Instance instance) -> DebugUtilsMessenger;
}