#pragma once
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include "core/frontend/window.hpp"
namespace render::vulkan {
auto createSurface(vk::Instance instance, const core::frontend::BaseWindow::WindowSystemInfo& wsi)
    -> vk::SurfaceKHR;
}
