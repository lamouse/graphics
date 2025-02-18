#include "SDL_common.hpp"
#include <cstring>
namespace graphics {
auto get_window_system_info() -> core::frontend::WindowSystemType {
    const auto* platform = SDL_GetPlatform();
    if (strcmp(platform, "Windows") == 0) {
        return core::frontend::WindowSystemType::Windows;
    }
    if (strcmp(platform, "Mac OS X") == 0) {
        return core::frontend::WindowSystemType::Cocoa;
    }
    if (strcmp(platform, "Linux") == 0) {
        return core::frontend::WindowSystemType::Cocoa;
    }
    return core::frontend::WindowSystemType::Headless;
}

auto get_windows_handles(SDL_Window* window) -> void* {
#if defined(SDL_PLATFORM_WIN32)
    return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                  SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

#elif defined(SDL_PLATFORM_MACOS)
    NSWindow* nswindow = (__bridge NSWindow*)SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    if (nswindow) {
        ...
    }
#elif defined(SDL_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        Display *xdisplay = (Display *)SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        Window xwindow = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(window),
                                                       SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        if (xdisplay && xwindow) {
            ...
        }
    } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
        struct wl_display *display = (struct wl_display *)SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        struct wl_surface *surface = (struct wl_surface *)SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
        if (display && surface) {
            ...
        }
    }
#elif defined(SDL_PLATFORM_IOS)
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    UIWindow* uiwindow = (__bridge UIWindow*)SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, NULL);
    if (uiwindow) {
        GLuint framebuffer = (GLuint)SDL_GetNumberProperty(
            props, SDL_PROP_WINDOW_UIKIT_OPENGL_FRAMEBUFFER_NUMBER, 0);
        GLuint colorbuffer = (GLuint)SDL_GetNumberProperty(
            props, SDL_PROP_WINDOW_UIKIT_OPENGL_RENDERBUFFER_NUMBER, 0);
        GLuint resolveFramebuffer = (GLuint)SDL_GetNumberProperty(
            props, SDL_PROP_WINDOW_UIKIT_OPENGL_RESOLVE_FRAMEBUFFER_NUMBER, 0);
        ...
    }
#endif
}
}  // namespace graphics