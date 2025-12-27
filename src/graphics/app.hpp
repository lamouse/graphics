
#pragma once
#include <memory>
#include "common/class_traits.hpp"
#include "QT_window.hpp"
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
        std::unique_ptr<QTWindow> qt_main_window;

        std::shared_ptr<core::System> sys_;
};
}  // namespace graphics
