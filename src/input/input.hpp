#pragma once
#include <memory>
namespace graphics::input {
class Mouse;
class Keyboard;
class InputSystem {
    public:
        auto GetMouse() -> Mouse *;
        auto GetKeyboard() -> Keyboard *;
        InputSystem();
        /**
         * @brief 使用之前必须调用否则GetMouse()等将返回nullptr，
            提供这个函数主要是进行渲染切换时重置输入状态
         *
         */
        void Init();
        void Shutdown();
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