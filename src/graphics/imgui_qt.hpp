#pragma once

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
namespace imgui::qt {
void mouse_press_event(QMouseEvent* event);
void mouse_release_event(QMouseEvent* event);
void mouse_move_event(QMouseEvent* event);
void mouse_wheel_event(QWheelEvent* event);
void mouse_focus_in_event();
void mouse_focus_out_event();
void new_frame(float weight, float height);
void key_press(QKeyEvent* event);
void key_release(QKeyEvent* event);
}  // namespace imgui::qt
