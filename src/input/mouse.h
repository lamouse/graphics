#pragma once
#include "input/input_engine.hpp"
#include <glm/glm.hpp>
namespace graphics::input {
enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
    Backward = 3,
    Forward = 4,
    UnDefined = -1
};

class Mouse final : public InputEngine<MouseButton> {
        struct Axis {
                float x;
                float y;
        };

    public:
        Mouse(const Mouse&) = delete;
        Mouse(Mouse&&) = delete;
        auto operator=(const Mouse&) -> Mouse& = delete;
        auto operator=(Mouse&&) -> Mouse& = delete;
        Mouse(std::string_view engine_name);
        ~Mouse() override = default;

        void MouseButtonPress(const glm::vec2& position, MouseButton button);
        void MouseButtonPress(MouseButton button);
        void MouseButtonRelease(MouseButton button);

        void MouseMove(const glm::vec2& position);
        void Move(int x, int y, int center_x, int center_y);
        void MouseScroll(const glm::vec2& offset);
        [[nodiscard]] auto GetAxis() const -> Axis { return axis_; }
        [[nodiscard]] auto IsButtonPressed() const -> bool;
        [[nodiscard]] auto GetMouseOrigin() const -> glm::vec2;
        [[nodiscard]] auto GetLastPosition() const -> glm::vec2;
        [[nodiscard]] auto GetLastMotionChange() const -> glm::vec2;
        [[nodiscard]] auto GetScrollOffset() const -> glm::vec2;

    private:
        auto IsMousePanningEnabled() -> bool;
        Axis axis_{.x = 0.0f, .y = 0.0f};
        bool button_pressed_{false};
        glm::vec2 mouse_origin_{};
        glm::vec2 last_position_{};
        glm::vec3 last_motion_change{};
        glm::vec2 last_change_{};
        // x: horizontal, y: vertical
        glm::vec2 scroll_offset_{};
};
}  // namespace graphics::input