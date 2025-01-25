#pragma once
#include <GLFW/glfw3.h>

#include <core/frontend/window.hpp>

namespace GLFWCommon {
auto get_window_system_info() -> core::frontend::WindowSystemType;

auto get_windows_handles(GLFWwindow* window) -> void*;

/**
 * @brief 初始化imgui vulkan
 *
 * @param window
 */
void init_glfw_imgui(GLFWwindow* window);

}  // namespace GLFWCommon
