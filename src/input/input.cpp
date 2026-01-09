#include "input/input.hpp"
#include "input/mouse.h"
#include "input/keyboard.hpp"
#include "input/drop.hpp"
namespace graphics::input {
struct InputSystem::Impl {
        template <typename Engine>
        void RegisterEngine(const std::string& name, std::shared_ptr<Engine>& engine) {
            engine = std::make_shared<Engine>(name);
        }

        template <typename Engine>
        void UnRegisterEngine(std::shared_ptr<Engine>& engine) {
            engine.reset();
        }

        void ShutDown() {
            UnRegisterEngine(mouse);
            UnRegisterEngine(keyboard_);
            UnRegisterEngine(file_drop_);
        }
        Impl() {
            RegisterEngine<Mouse>("Mouse", mouse);
            RegisterEngine<Keyboard>("Keyboard", keyboard_);
            RegisterEngine("file drop", file_drop_);
        }

        std::shared_ptr<Mouse> mouse;
        std::shared_ptr<Keyboard> keyboard_;
        std::shared_ptr<FileDrop> file_drop_;
};

InputSystem::InputSystem() : impl_(nullptr) {}
void InputSystem::Init() { impl_ = std::make_unique<Impl>(); }
void InputSystem::Shutdown() { impl_->ShutDown(); }
InputSystem::~InputSystem() = default;
auto InputSystem::GetMouse() -> Mouse* { return impl_->mouse.get(); }
auto InputSystem::GetFileDrop() -> FileDrop*{
    return impl_->file_drop_.get();
}

auto InputSystem::GetKeyboard() -> Keyboard* { return impl_->keyboard_.get(); }
}  // namespace graphics::input