#include "SDL_common.hpp"
#include <cstring>
#ifdef __linux__
#include <X11/Xlib.h>
#endif
namespace graphics {
auto get_window_system_info() -> core::frontend::WindowSystemType {
    const auto *platform = SDL_GetPlatform();
    if (strcmp(platform, "Windows") == 0) {
        return core::frontend::WindowSystemType::Windows;
    }
    if (strcmp(platform, "Mac OS X") == 0) {
        return core::frontend::WindowSystemType::Cocoa;
    }
    if (strcmp(platform, "Linux") == 0) {
        const char *video_driver = SDL_GetCurrentVideoDriver();
        if (strcmp(video_driver, "x11") == 0) {
            // 使用的是 X11
            return core::frontend::WindowSystemType::X11;
        }
        if (strcmp(video_driver, "wayland") == 0) {
            // 使用的是 Wayland
            return core::frontend::WindowSystemType::Wayland;
        }
    }
    return core::frontend::WindowSystemType::Headless;
}

auto get_windows_handles(SDL_Window *window) -> void * {
#if defined(SDL_PLATFORM_WIN32)
    return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                  SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

#elif defined(SDL_PLATFORM_MACOS)
    NSWindow *nswindow = (__bridge NSWindow *)SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    if (nswindow) {
        return nswindow;
    }
    return nullptr;
#elif defined(SDL_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        return reinterpret_cast<void *>(SDL_GetNumberProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
    }
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
        return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                      SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    }
#endif
    return nullptr;
}

#ifdef __linux__
auto get_windows_display(SDL_Window *window) -> void * {
#if defined(SDL_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                      SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    }
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
        return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                      SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
    }
#endif
    return nullptr;
}

#endif

}  // namespace graphics