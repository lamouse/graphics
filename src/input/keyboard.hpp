#pragma once
#include "input/keys.hpp"
#include "input/input_engine.hpp"
namespace graphics::input {

class Keyboard : public InputEngine<NativeKeyboard::Keys> {
    public:
        explicit Keyboard(std::string_view engine_name);

        /**
         * Sets the status of all buttons bound with the key to pressed
         * @param key_code the code of the key to press
         */
        void PressKey(NativeKeyboard::Keys key_code);

        /**
         * Sets the status of all buttons bound with the key to released
         * @param key_code the code of the key to release
         */
        void ReleaseKey(NativeKeyboard::Keys key_code);

        /**
         * Sets the status of all keyboard modifier keys
         * @param key_modifiers the code of the key to release
         */
        void SetKeyboardModifiers(NativeKeyboard::Modifiers key_modifiers);

        /// Sets all keys to the non pressed state
        void ReleaseAllKeys();
        auto HasModifiers(NativeKeyboard::Modifiers modifiers) -> bool {
            return NativeKeyboard::hasFlag(modifiers_, modifiers);
        }

    private:
        NativeKeyboard::Modifiers modifiers_{NativeKeyboard::Modifiers::None};
};
}  // namespace graphics::input