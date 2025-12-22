#pragma once
#include <memory>
namespace graphics::input {
class Mouse;
class InputSystem {
    public:
        auto GetMouse() -> Mouse*;
        InputSystem();
        InputSystem(const InputSystem &) = delete;
        InputSystem(InputSystem &&) = delete;
        auto operator=(const InputSystem &) -> InputSystem & = delete;
        auto operator=(InputSystem &&) -> InputSystem & = delete;
        ~InputSystem();

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
};
}  // namespace graphics::input