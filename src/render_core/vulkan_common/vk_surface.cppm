module;
export module render.vulkan.common.surface;
import render.vulkan.common.wrapper;
import core;

namespace render::vulkan {

export auto createSurface(const Instance& instance, const core::frontend::BaseWindow::WindowSystemInfo& wsi)
    -> SurfaceKHR;
}