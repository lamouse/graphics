#include "input/mouse.h"
namespace graphics::input {

constexpr float default_deadzone_counterweight = 0.01f;
constexpr float default_panning_sensitivity = 0.0010f;
constexpr float default_stick_sensitivity = 0.0006f;

Mouse::Mouse(std::string_view engine_name) : InputEngine<MouseButton>(engine_name) {};

void Mouse::PressButton(const glm::vec2& position, MouseButton button) {
    SetButtonState(button, button_pressed_);
    std::scoped_lock lock(mutex_);
    button_pressed_ = true;
    mouse_origin_ = position;
    last_position_ = position;
}

void Mouse::PressMouseButton(MouseButton button) {
    SetButtonState(button, button_pressed_);
    std::scoped_lock lock(mutex_);
    button_pressed_ = true;
}

void Mouse::ReleaseButton(MouseButton button) {
    SetButtonState(button, button_pressed_);
    std::scoped_lock lock(mutex_);
    button_pressed_ = false;
    if (!IsMousePanningEnabled()) {
        axis_.x = 0.0f;
        axis_.y = 0.0f;
    }
    last_motion_change.x = 0;
    last_motion_change.y = 0;
}

auto Mouse::IsButtonPressed() const -> bool {
    std::scoped_lock lock(mutex_);
    return button_pressed_;
}

void Mouse::MouseMove(const glm::vec2& position) {
    std::scoped_lock lock(mutex_);
    relative = position - glm::vec2(axis_.x, axis_.y);
    axis_.x = position.x;
    axis_.y = position.y;
}

void Mouse::Move(int x, int y, int center_x, int center_y) {
    std::scoped_lock lock(mutex_);
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
    if (button_pressed_) {
        const auto mouse_move = glm::vec2{x, y} - mouse_origin_;
        const float x_sensitivity = default_stick_sensitivity;
        const float y_sensitivity = default_stick_sensitivity;
        // axis_.x = static_cast<float>(mouse_move.x) * x_sensitivity;
        // axis_.y = static_cast<float>(-mouse_move.y) * y_sensitivity;

        last_motion_change = {
            static_cast<float>(-mouse_move.x) * x_sensitivity,
            static_cast<float>(-mouse_move.y) * y_sensitivity,
            last_motion_change.z,
        };
    }
}
// TODO implement
auto Mouse::IsMousePanningEnabled() -> bool {
    return false;
}
void Mouse::Scroll(const glm::vec2& offset) {
    std::scoped_lock lock(mutex_);
    scroll_offset_ += offset;
    if (scroll_offset_.y > 45.f) {
        scroll_offset_.y = 45;
    }
    if (scroll_offset_.y < 0.f) {
        scroll_offset_.y = 0.f;
    }
}

auto Mouse::GetMouseOrigin() const -> glm::vec2 {
    std::scoped_lock lock(mutex_);
    return mouse_origin_;
}
auto Mouse::GetLastPosition() const -> glm::vec2 {
    std::scoped_lock lock(mutex_);
    return last_position_;
}
auto Mouse::GetLastMotionChange() const -> glm::vec2 {
    std::scoped_lock lock(mutex_);
    return last_motion_change;
}
auto Mouse::GetScrollOffset() const -> glm::vec2 {
    std::scoped_lock lock(mutex_);
    return scroll_offset_;
}
auto Mouse::GetRelative() const -> glm::vec2 {
    std::scoped_lock lock(mutex_);
    return relative;
}
auto Mouse::popRelative() -> glm::vec2 {
    std::scoped_lock lock(mutex_);
    auto tmp = relative;
    relative = glm::vec2{.0f};
    return tmp;
}
void Mouse::ReleaseAllButtons() {
    ResetButtonState();
    std::scoped_lock lock(mutex_);
    button_pressed_ = false;
}
}  // namespace graphics::input