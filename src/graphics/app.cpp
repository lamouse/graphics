#include "app.hpp"
#include "input/input.hpp"
#include <thread>

namespace graphics {

App::App() {
    input_system_ = std::make_shared<input::InputSystem>();

    sdl_window = std::make_unique<SDLWindow>(input_system_, 1920, 1080, "graphics");
    sys_ = std::make_shared<core::System>();
    sys_->load(*sdl_window);
    std::jthread render_thread_ = std::jthread([this](std::stop_token token) {
        while (!token.stop_requested()) {
            sys_->run(input_system_);
        }
        sys_->shutdownMainProcess();
    });
    while (!sdl_window->shouldClose()) {
        sdl_window->pullEvents();
    }
}

App::~App() = default;

}  // namespace graphics
