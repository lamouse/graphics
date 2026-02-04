
#pragma once
#include <memory>
#include "graphics/sdl_window.hpp"
#include <core/core.hpp>

namespace graphics {
namespace input {
class InputSystem;
}
class RenderRegistry;
class App {
    public:
        explicit App();
        App(const App&) = delete;
        App(App&&) noexcept = delete;
        auto operator=(const App&) -> App& = delete;
        auto operator=(App&&) noexcept -> App& = delete;
        ~App();

    private:
        std::shared_ptr<input::InputSystem> input_system_;
        std::unique_ptr<SDLWindow> sdl_window;
        std::shared_ptr<core::System> sys_;
};
}  // namespace graphics
