#include "sdl_window.hpp"
#include <fmt/format.h>
#include <stdexcept>
#include "SDL_common.hpp"
#include "imgui_impl_sdl3.h"
#include <imgui.h>
namespace graphics {
SDLWindow::SDLWindow(int width, int height, std::string_view title) {
    core::frontend::BaseWindow::WindowConfig conf;
    conf.extent.width = width;
    conf.extent.height = height;
    conf.fullscreen = false;
    setWindowConfig(conf);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error(
            fmt::format("SDL could not initialize! SDL_Error: {}\n", SDL_GetError()));
    }
    int const w = static_cast<int>((float)conf.extent.width * window_info.render_surface_scale);
    int const h = static_cast<int>((float)conf.extent.height * window_info.render_surface_scale);
    // Create window with Vulkan graphics context
    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
                                          SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
    window_ = SDL_CreateWindow(title.data(), w, h, window_flags);
    if (window_ == nullptr) {
        throw std::runtime_error(
            fmt::format("SDL could not initialize! SDL_Error: {}\n", SDL_GetError()));
    }
#if defined(SDL_PLATFORM_MACOS)
    window_info.render_surface_scale = 1.0f;
#else
    window_info.render_surface_scale = SDL_GetWindowDisplayScale(window_);
#endif

    if (window_info.render_surface_scale == 0) {
        throw std::runtime_error(
            fmt::format("SDL_GetWindowDisplayScale! SDL_Error: {}\n", SDL_GetError()));
    }
    window_info.type = get_window_system_info();
    window_info.render_surface = get_windows_handles(window_);
#ifdef __linux__
    window_info.display_connection = get_windows_display(window_);
#endif
    // 添加事件监视器
    SDL_AddEventWatch(
        [](void* userdata, SDL_Event* event) -> bool {
            if (event->type == SDL_EVENT_WINDOW_RESIZED) {
                auto* base_window = reinterpret_cast<SDLWindow*>(userdata);
                if (event->window.windowID == SDL_GetWindowID(base_window->getWindow())) {
                    base_window->setWindowConfig(
                        WindowConfig{.extent = {.width = static_cast<int>(event->window.data1),
                                                .height = static_cast<int>(event->window.data2)}});
                    // base_window->UpdateCurrentFramebufferLayout(
                    //     static_cast<uint32_t>(event->window.data1),
                    //     static_cast<uint32_t>(event->window.data2));
                }
            }
            return false;
        },
        this);
    // 设置窗口的最小和最大尺寸
    SDL_SetWindowMinimumSize(window_, 160, 90);  // 最小尺寸
    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window_);
}

auto SDLWindow::IsMinimized() const -> bool {  // 获取窗口的标志
    SDL_WindowFlags flags = SDL_GetWindowFlags(window_);
    // 检查是否包含 SDL_WINDOW_MINIMIZED 标志
    return (flags & SDL_WINDOW_MINIMIZED) != 0;
}

auto SDLWindow::IsShown() const -> bool {  // 获取窗口的标志
    auto flags = SDL_GetWindowFlags(window_);
    // 检查是否包含 SDL_WINDOW_SHOWN 标志
    return (flags & SDL_WINDOW_HIDDEN) == 0;
}

auto SDLWindow::shouldClose() const -> bool { return should_close_; }
void SDLWindow::setWindowTitle(std::string_view title) {
    SDL_SetWindowTitle(window_, title.data());
}
void SDLWindow::configGUI() { ImGui_ImplSDL3_InitForVulkan(window_); }
void SDLWindow::destroyGUI() { ImGui_ImplSDL3_Shutdown(); }
void SDLWindow::newFrame() { ImGui_ImplSDL3_NewFrame(); }
void SDLWindow::pullEvents() {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL3_ProcessEvent(&e);
        if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
            e.window.windowID == SDL_GetWindowID(window_)) {
            should_close_ = true;
        }

        switch (e.type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                if (e.window.windowID == SDL_GetWindowID(window_)) {
                    should_close_ = true;
                }
                break;
            }
            case SDL_EVENT_KEY_UP: {
                if (e.key.repeat == 0) {
                }
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                if (e.button.button >= 1 && e.button.button <= 3) {
                }
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (e.button.button >= 1 && e.button.button <= 3) {
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                break;
            }
        }
    }
}
SDLWindow::~SDLWindow() {
    if (window_) {
        SDL_DestroyWindow(window_);
        SDL_Quit();
    }
}
}  // namespace graphics