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
void SDLWindow::pullEvents(core::InputEvent& event) {
    SDL_Event e;

    const std::vector<SDL_Scancode> keys_to_check = {
        SDL_SCANCODE_W,     SDL_SCANCODE_A,     SDL_SCANCODE_S,
        SDL_SCANCODE_D,     SDL_SCANCODE_SPACE, SDL_SCANCODE_LEFT,
        SDL_SCANCODE_RIGHT, SDL_SCANCODE_DOWN,  SDL_SCANCODE_UP};
    const auto* keyboardState = SDL_GetKeyboardState(nullptr);

    for (auto sc : keys_to_check) {
        if (keyboardState[sc]) {
            auto key = transform_SDL_Key(sc);
            if (key != core::InputKey::UN_SUPER) {
                core::InputState state{};
                state.key = key;
                state.key_down.Assign(1);  // 持续推送“按下”事件
                event.push_event(state);
            }
        }
    }

    float mx{}, my{};
    Uint32 mouseState = SDL_GetMouseState(&mx, &my);
    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;
    SDL_GetRelativeMouseState(&lastMouseX_, &lastMouseY_);
    bool leftButton = mouseState & SDL_BUTTON_MASK(SDL_BUTTON_LEFT);
    bool rightButton = mouseState & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT);
    bool middleButton = mouseState & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE);
    if (leftButton) {
        core::InputState state{};
        state.mouse_button_left.Assign(1);
        state.mouseX_ = mx;
        state.mouseY_ = my;
        state.key_down.Assign(1);
        state.mouseRelativeX_ = lastMouseX_;
        state.mouseRelativeY_ = lastMouseY_;
        event.push_event(state);
    }
    if (rightButton) {
        core::InputState state{};
        state.mouse_button_right.Assign(1);
        state.mouseX_ = mx;
        state.mouseY_ = my;
        state.mouseRelativeX_ = lastMouseX_;
        state.mouseRelativeY_ = lastMouseY_;
        state.key_down.Assign(1);
        event.push_event(state);
    }
    if (middleButton) {
        core::InputState state{};
        state.mouse_button_center.Assign(1);
        state.mouseX_ = mx;
        state.mouseY_ = my;
        state.mouseRelativeX_ = lastMouseX_;
        state.mouseRelativeY_ = lastMouseY_;
        state.key_down.Assign(1);
        event.push_event(state);
    }

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
                    if (transform_SDL_Key(e.key.scancode) != core::InputKey::UN_SUPER) {
                        core::InputState input_state;
                        input_state.key_up.Assign(1);
                        input_state.key = transform_SDL_Key(e.key.scancode);
                        event.push_event(input_state);
                    }
                }
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                core::InputState input_state;
                input_state.mouseX_ = e.motion.x;
                input_state.mouseY_ = e.motion.y;
                input_state.mouse_move.Assign(1);
                event.push_event(input_state);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                if (e.button.button >= 1 && e.button.button <= 3) {
                    core::InputState input_state;
                    input_state.key_up.Assign(1);
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        input_state.mouse_button_left.Assign(1);
                    }
                    if (e.button.button == SDL_BUTTON_MIDDLE) {
                        input_state.mouse_button_center.Assign(1);
                    }
                    if (e.button.button == SDL_BUTTON_RIGHT) {
                        input_state.mouse_button_right.Assign(1);
                    }
                    event.push_event(input_state);
                }
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (e.button.button >= 1 && e.button.button <= 3) {
                    core::InputState input_state;
                    input_state.key_down.Assign(1);
                    input_state.first_down.Assign(1);
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        input_state.mouse_button_left.Assign(1);
                    }
                    if (e.button.button == SDL_BUTTON_MIDDLE) {
                        input_state.mouse_button_center.Assign(1);
                    }
                    if (e.button.button == SDL_BUTTON_RIGHT) {
                        input_state.mouse_button_right.Assign(1);
                    }
                    input_state.mouseX_ = e.motion.x;
                    input_state.mouseY_ = e.motion.y;
                    event.push_event(input_state);
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                core::InputState input_state;
                const auto* sdl_key_state = SDL_GetKeyboardState(nullptr);
                if (sdl_key_state[SDL_SCANCODE_LCTRL]) {
                    // 左 Ctrl 被按下
                    input_state.key = core::InputKey::LCtrl;
                    input_state.key_down.Assign(1);
                }
                input_state.scrollOffset_ += e.wheel.y;
                event.push_event(input_state);
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