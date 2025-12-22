#pragma once
#include "core/frontend/window.hpp"
#include <QMouseEvent>

class QWindow;

namespace graphics {
namespace input {
enum class MouseButton;
}
namespace qt {
auto get_window_system_type() -> core::frontend::WindowSystemType;
auto get_window_system_info(QWindow* window) -> core::frontend::BaseWindow::WindowSystemInfo;

auto QtButtonToMouseButton(Qt::MouseButton button) -> input::MouseButton;
}  // namespace qt
}  // namespace graphics