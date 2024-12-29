#pragma once
#include <vulkan/vulkan.hpp>
namespace render::vulkan {
[[nodiscard]] auto createDebugMessenger(::vk::Instance& instance) -> ::vk::DebugUtilsMessengerEXT;
}