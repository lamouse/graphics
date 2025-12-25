module;

#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include "core/frontend/window.hpp"
export module render.vulkan.common.surface;
namespace render::vulkan {
export auto createSurface(vk::Instance instance, const core::frontend::BaseWindow::WindowSystemInfo& wsi)
    -> SurfaceKHR;
}