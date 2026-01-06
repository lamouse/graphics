#pragma once
#include <SDL3/SDL.h>
#include "core/frontend/window.hpp"
#include "input/keys.hpp"
namespace graphics {
auto get_window_system_info() -> core::frontend::WindowSystemType;

auto get_window_info(SDL_Window* window) -> core::frontend::BaseWindow::WindowSystemInfo;

auto FromSDLKeymod(SDL_Keymod sdlMod) -> input::NativeKeyboard::Modifiers;

auto FromSDLKeycode(SDL_Keycode code) -> input::NativeKeyboard::Keys;

}  // namespace graphics