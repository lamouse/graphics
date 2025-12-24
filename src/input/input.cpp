#include "input/input.hpp"
#include "input/mouse.h"
namespace graphics::input {
struct InputSystem::Impl {
        template <typename Engine>
        void RegisterEngine(const std::string& name, std::shared_ptr<Engine>& engine) {
            engine = std::make_shared<Engine>(name);
        }

        template<typename Engine>
        void UnRegisterEngine(std::shared_ptr<Engine>& engine){
            engine.reset();
        }

        void ShutDown(){
            UnRegisterEngine(mouse);
        }
        Impl() { RegisterEngine<Mouse>("Mouse", mouse); }

        std::shared_ptr<Mouse> mouse;
};

InputSystem::InputSystem():impl_(nullptr) {}
void InputSystem::Init(){
    impl_ = std::make_unique<Impl>();
}
void InputSystem::Shutdown(){
    impl_->ShutDown();
}
InputSystem::~InputSystem() = default;
auto InputSystem::GetMouse() -> Mouse* { return impl_->mouse.get(); }
}  // namespace graphics::input