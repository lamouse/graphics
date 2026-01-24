#include "input/keyboard.hpp"

namespace graphics::input {
Keyboard::Keyboard(std::string_view engine_name) : InputEngine<NativeKeyboard::Keys>(engine_name) {}

void Keyboard::PressKey(NativeKeyboard::Keys key_code) { SetButtonState(key_code, true); }

void Keyboard::ReleaseKey(NativeKeyboard::Keys key_code) { SetButtonState(key_code, false); }

void Keyboard::SetKeyboardModifiers(NativeKeyboard::Modifiers key_modifiers) {
    std::scoped_lock lock(mutex_);
    modifiers_ = key_modifiers;
}

void Keyboard::ReleaseAllKeys() {
    ResetButtonState();
    std::scoped_lock lock(mutex_);
    modifiers_ = NativeKeyboard::Modifiers::None;
}

}  // namespace graphics::input