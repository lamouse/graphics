module;
#include "core/frontend/window.hpp"
#include <vulkan/vulkan.hpp>
export module render.vulkan.common.surface;
import render.vulkan.common.wrapper;

namespace render::vulkan {

export auto createSurface(vk::Instance instance, const core::frontend::BaseWindow::WindowSystemInfo& wsi)
    -> SurfaceKHR;
}