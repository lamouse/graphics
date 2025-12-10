#pragma once
#include "common/bit_field.hpp"
#include <queue>
#include <optional>
namespace core {

enum class InputKey : std::int16_t {
    UN_SUPER = -1,
    Space,
    W,
    A,
    S,
    D,
    Left,
    Right,
    Down,
    Up,
    Insert,
    Esc,
    LCtrl,
};

struct InputState {
        InputKey key{InputKey::UN_SUPER};
        union {
                std::uint8_t raw1{0};
                BitField<0, 1, std::uint8_t> mouse_button_left;
                BitField<1, 1, std::uint8_t> mouse_button_right;
                BitField<2, 1, std::uint8_t> mouse_button_center;
                BitField<3, 1, std::uint8_t> mouse_captured;
                BitField<4, 1, std::uint8_t> key_down;
                BitField<5, 1, std::uint8_t> key_up;
                BitField<6, 1, std::uint8_t> mouse_move;
                BitField<7, 1, std::uint8_t> first_down;
        };
        auto operator==(const InputState& other) const -> bool {
            return key == other.key && raw1 == other.raw1;
        }

        // 重载 == 运算符（成员函数）
        auto operator!=(const InputState& other) const -> bool { return !(*this == other); }

        float mouseX_ = -1.0f, mouseY_ = -1.0f;
        float mouseRelativeX_{0}, mouseRelativeY_{0};
        float scrollOffset_ = 0.0f;

        using IsDown = bool;
        using FirstDown = bool;
        [[nodiscard]] auto mouseLeftButtonDown() const -> std::pair<IsDown, FirstDown> {
            bool down{}, first{};
            if (mouse_button_left && key_down) {
                down = true;
            }
            if (first_down) {
                first = true;
            }
            return std::make_pair(down, first);
        }

        [[nodiscard]] auto onlyMouseMove() const -> bool {
            return mouse_move && !mouse_button_left && !mouse_button_right &&
                   !mouse_button_center && key == InputKey::UN_SUPER;
        }

        [[nodiscard]] auto mouseLeftButtonUp() const -> bool { return mouse_button_left && key_up; }

        [[nodiscard]] auto keyDown(InputKey key_) const -> bool {
            if (key_ == key) {
                return static_cast<bool>(key_down);
            }
            return false;
        }
};

class InputEvent {
    public:
        void push_event(const InputState& state) { input_queue.push(state); }
        auto pop_event() -> std::optional<InputState> {
            if (input_queue.empty()) {
                return std::nullopt;
            }
            auto ret = std::optional(input_queue.front());
            input_queue.pop();
            return ret;
        }
        auto empty() -> bool { return input_queue.empty(); }

    private:
        std::queue<InputState> input_queue;
};

}  // namespace core