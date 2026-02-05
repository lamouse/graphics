#include "app.hpp"
#include "input/input.hpp"

namespace graphics {

App::App() {
    input_system_ = std::make_shared<input::InputSystem>();

    sdl_window = std::make_unique<SDLWindow>(input_system_, 1920, 1080, "graphics");
    sys_ = std::make_shared<core::System>();
    sys_->load(*sdl_window);

    while (!sdl_window->shouldClose()) {
        sdl_window->pullEvents();
        sys_->run(input_system_);
    }
}

App::~App() = default;

}  // namespace graphics
