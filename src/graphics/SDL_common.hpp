#pragma once
#include <SDL3/SDL.h>
#include "core/frontend/window.hpp"
namespace graphics {
auto get_window_system_info() -> core::frontend::WindowSystemType;

auto get_windows_handles(SDL_Window* window) -> void*;

#ifdef __linux__
auto get_windows_display(SDL_Window* window) -> void*;
#endif

}  // namespace graphics