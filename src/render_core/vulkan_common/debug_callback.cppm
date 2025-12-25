module;
#include<vulkan/vulkan.hpp>

export module render.vulkan.common.DebugMessenger;
import render.vulkan.common.wrapper;

namespace render::vulkan {
export [[nodiscard]] auto createDebugMessenger(::vk::Instance instance) -> DebugUtilsMessenger;
}

