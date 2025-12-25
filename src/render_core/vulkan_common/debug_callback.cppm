module;
#include "vulkan_wrapper.hpp"

export module render;
namespace render::vulkan {
export [[nodiscard]] auto createDebugMessenger(::vk::Instance instance) -> DebugUtilsMessenger;
}

