#pragma once
#include <GLFW/glfw3.h>

#include <core/frontend/window.hpp>

namespace GLFWCommon {
auto get_window_system_info() -> core::frontend::WindowSystemType;

auto get_windows_handles(GLFWwindow* window) -> void*;

}  // namespace GLFWCommon
