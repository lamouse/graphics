#pragma once
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include "core/frontend/window.hpp"
#include "vulkan_wrapper.hpp"
namespace render::vulkan {
auto createSurface(vk::Instance instance, const core::frontend::BaseWindow::WindowSystemInfo& wsi)
    -> SurfaceKHR;
}
