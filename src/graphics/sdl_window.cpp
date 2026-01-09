#include "sdl_window.hpp"
#include <fmt/format.h>
#include <stdexcept>
#include <utility>
#include "SDL_common.hpp"
#include "imgui_impl_sdl3.h"
#include "input/input.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.h"
#include "input/drop.hpp"
#include <imgui.h>

namespace graphics {

namespace {

void process_mouse_press(SDL_Event e, graphics::input::Mouse* mouse) {
    switch (e.button.button) {
        case SDL_BUTTON_LEFT: {
            mouse->PressMouseButton(input::MouseButton::Left);
            mouse->PressButton(glm::vec2{e.motion.x, e.motion.y}, input::MouseButton::Left);
            break;
        }
        case SDL_BUTTON_RIGHT: {
            mouse->PressMouseButton(input::MouseButton::Right);
            mouse->PressButton(glm::vec2{e.motion.x, e.motion.y}, input::MouseButton::Right);
            break;
        }
        case SDL_BUTTON_MIDDLE: {
            mouse->PressMouseButton(input::MouseButton::Middle);
            mouse->PressButton(glm::vec2{e.motion.x, e.motion.y}, input::MouseButton::Middle);
            break;
        }
        case SDL_BUTTON_X1: {
            mouse->PressMouseButton(input::MouseButton::Forward);
            mouse->PressButton(glm::vec2{e.motion.x, e.motion.y}, input::MouseButton::Forward);
            break;
        }
        case SDL_BUTTON_X2: {
            mouse->PressMouseButton(input::MouseButton::Backward);
            mouse->PressButton(glm::vec2{e.motion.x, e.motion.y}, input::MouseButton::Backward);
            break;
        }
        default:
            break;
    }
}

void process_mouse_release(SDL_Event e, graphics::input::Mouse* mouse) {
    switch (e.button.button) {
        case SDL_BUTTON_LEFT: {
            mouse->ReleaseButton(input::MouseButton::Left);
            break;
        }
        case SDL_BUTTON_RIGHT: {
            mouse->ReleaseButton(input::MouseButton::Right);
            break;
        }
        case SDL_BUTTON_MIDDLE: {
            mouse->ReleaseButton(input::MouseButton::Middle);
            break;
        }
        case SDL_BUTTON_X1: {
            mouse->ReleaseButton(input::MouseButton::Forward);
            break;
        }
        case SDL_BUTTON_X2: {
            mouse->ReleaseButton(input::MouseButton::Backward);
            break;
        }
        default:
            break;
    }
}

}  // namespace

SDLWindow::SDLWindow(std::shared_ptr<input::InputSystem> input_system, int width, int height,
                     std::string_view title)
    : input_system_(std::move(input_system)) {
    input_system_->Init();
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
    window_info = get_window_info(window_);
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
                auto* key_board = input_system_->GetKeyboard();
                key_board->SetKeyboardModifiers(FromSDLKeymod(e.key.mod));
                key_board->ReleaseKey(FromSDLKeycode(e.key.key));
                break;
            }
            case SDL_EVENT_KEY_DOWN: {
                auto* key_board = input_system_->GetKeyboard();
                key_board->SetKeyboardModifiers(FromSDLKeymod(e.key.mod));
                key_board->PressKey(FromSDLKeycode(e.key.key));
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                auto* mouse = input_system_->GetMouse();
                mouse->MouseMove(glm::vec2(e.motion.x, e.motion.y));
                int w{}, h{};
                SDL_GetWindowSize(window_, &w, &h);

                mouse->Move(static_cast<int>(e.motion.x), static_cast<int>(e.motion.y), w / 2,
                            h / 2);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                if (e.button.button >= SDL_BUTTON_LEFT && e.button.button <= SDL_BUTTON_X2) {
                    auto* mouse = input_system_->GetMouse();
                    process_mouse_release(e, mouse);
                }
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (e.button.button >= SDL_BUTTON_LEFT && e.button.button <= SDL_BUTTON_X2) {
                    auto* mouse = input_system_->GetMouse();
                    process_mouse_press(e, mouse);
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                auto* mouse = input_system_->GetMouse();
                mouse->Scroll(glm::vec2(e.wheel.x, e.wheel.y));
                break;
            }
            case SDL_EVENT_DROP_FILE:{
                const char* fileName = e.drop.data;
                auto* file_drop = input_system_->GetFileDrop();
                file_drop->push(fileName);
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