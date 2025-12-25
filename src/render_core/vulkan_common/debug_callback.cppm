module;
#include "vulkan_wrapper.hpp"

export module render.vulkan.common.DebugMessenger;
namespace render::vulkan {
export [[nodiscard]] auto createDebugMessenger(::vk::Instance instance) -> DebugUtilsMessenger;
}

