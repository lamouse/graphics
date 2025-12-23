#pragma once
#include <memory>

namespace render {
class RenderBase;
}
namespace core::frontend {
class BaseWindow;
}
namespace graphics {
    namespace input {
        class InputSystem;
    }
auto createWindow(std::shared_ptr<input::InputSystem> input_system) -> std::unique_ptr<core::frontend::BaseWindow>;
auto createRender(core::frontend::BaseWindow* window) -> std::unique_ptr<render::RenderBase>;
}  // namespace graphics