#include "input/mouse.h"
namespace graphics::input {

constexpr float default_deadzone_counterweight = 0.01f;
constexpr float default_panning_sensitivity = 0.0010f;
constexpr float default_stick_sensitivity = 0.0006f;

Mouse::Mouse(std::string_view engine_name) : InputEngine<MouseButton>(engine_name) {};

void Mouse::PressButton(const glm::vec2& position, MouseButton button) {
    button_pressed_ = true;
    SetButtonState(button, button_pressed_);
    mouse_origin_ = position;
    last_position_ = position;
}

void Mouse::PressMouseButton(MouseButton button) {
    button_pressed_ = true;
    SetButtonState(button, button_pressed_);
}

void Mouse::ReleaseButton(MouseButton button) {
    button_pressed_ = false;
    SetButtonState(button, button_pressed_);
    if (!IsMousePanningEnabled()) {
        axis_.x = 0.0f;
        axis_.y = 0.0f;
    }
    last_motion_change.x = 0;
    last_motion_change.y = 0;
}

auto Mouse::IsButtonPressed() const -> bool { return button_pressed_; }

void Mouse::MouseMove(const glm::vec2& position) {
    axis_.x = position.x;
    axis_.y = position.y;
}

void Mouse::Move(int x, int y, int center_x, int center_y) {
    if (IsMousePanningEnabled()) {
        const auto mouse_change = glm::vec2{x, y} - glm::vec2{center_x, center_y};
        const float x_sensitivity = default_panning_sensitivity;
        const float y_sensitivity = default_panning_sensitivity;
        const float deadzone_counterweight = default_deadzone_counterweight;

        last_motion_change +=
            glm::vec3{-mouse_change.y * x_sensitivity, -mouse_change.x * y_sensitivity, 0};
        last_change_.x += mouse_change.x * x_sensitivity;
        last_change_.y += mouse_change.y * y_sensitivity;

        // Bind the mouse change to [0 <= deadzone_counterweight <= 1.0]

        const float length = glm::length(last_change_);
        if (length < deadzone_counterweight && length != 0.0f) {
            last_change_ /= length;
            last_change_ *= deadzone_counterweight;
        }

        return;
    }
    if (IsButtonPressed()) {
        const auto mouse_move = glm::vec2{x, y} - mouse_origin_;
        const float x_sensitivity = default_stick_sensitivity;
        const float y_sensitivity = default_stick_sensitivity;
        axis_.x = static_cast<float>(mouse_move.x) * x_sensitivity;
        axis_.y = static_cast<float>(-mouse_move.y) * y_sensitivity;

        last_motion_change = {
            static_cast<float>(-mouse_move.y) * x_sensitivity,
            static_cast<float>(-mouse_move.x) * y_sensitivity,
            last_motion_change.z,
        };
    }
}
// TODO implement
auto Mouse::IsMousePanningEnabled() -> bool { return false; }
void Mouse::Scroll(const glm::vec2& offset) { scroll_offset_ += offset; }

auto Mouse::GetMouseOrigin() const -> glm::vec2 { return mouse_origin_; }
auto Mouse::GetLastPosition() const -> glm::vec2 { return mouse_origin_; }
auto Mouse::GetLastMotionChange() const -> glm::vec2 { return mouse_origin_; }
auto Mouse::GetScrollOffset() const -> glm::vec2 { return mouse_origin_; }
}  // namespace graphics::input