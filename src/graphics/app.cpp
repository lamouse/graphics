#include "app.hpp"

#include "input/input.hpp"

namespace graphics {

App::App() {
    input_system_ = std::make_shared<input::InputSystem>(),
    qt_main_window = std::make_unique<QTWindow>(input_system_, sys_, 1920, 1080, "graphics");
}

App::~App() = default;

}  // namespace graphics
