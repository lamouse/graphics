#pragma once
#include "common/bit_field.hpp"
#include <queue>
#include <optional>
namespace core {

    enum class InputKey : std::int16_t{
        UN_SUPER = -1,
        Space,
        W, A, S, D,
        Left, Right, Down, Up,
        Insert,
        Esc
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
        };
        float mouseX_ = -1.0f, mouseY_ = -1.0f;
        float mouseRelativeX_{0}, mouseRelativeY_{0};
        float scrollOffset_ = 0.0f;
};

class InputEvent {
    public:
        void push_event(const InputState& state) {input_queue.push(state);}
        auto pop_event() -> std::optional<InputState>{
            if(input_queue.empty()){
                return std::nullopt;
            }
            auto ret = std::optional(input_queue.front());
            input_queue.pop();
            return ret;
        }

    private:
        std::queue<InputState> input_queue;
};

}  // namespace core