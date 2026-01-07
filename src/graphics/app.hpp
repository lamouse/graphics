
#pragma once
#include <memory>
#include "common/class_traits.hpp"
#ifdef USE_QT
#include "QT_window.hpp"
#else
#include "graphics/sdl_window.hpp"
#endif
#include <core/core.hpp>

namespace graphics {
namespace input {
class InputSystem;
}
class RenderRegistry;
class App {
    public:
        explicit App();
        CLASS_NON_COPYABLE(App);
        CLASS_NON_MOVEABLE(App);
        ~App();

    private:
        std::shared_ptr<input::InputSystem> input_system_;
#ifdef USE_QT
        std::unique_ptr<QTWindow> qt_main_window;
#else
        std::unique_ptr<SDLWindow> sdl_window;
#endif
        std::shared_ptr<core::System> sys_;
};
}  // namespace graphics
