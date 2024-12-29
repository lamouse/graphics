#pragma once
#include "vulkan_common.hpp"
namespace render::vulkan {
auto setupDebugMessenger(::vk::Instance& instance) -> ::vk::DebugUtilsMessengerEXT;
}