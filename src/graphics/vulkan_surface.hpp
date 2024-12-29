#pragma once
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "core/frontend/window.hpp"
namespace vulkan {
auto createSurface(vk::Instance instance, const core::frontend::BaseWindow::WindowSystemInfo& wsi) -> vk::SurfaceKHR;
}
