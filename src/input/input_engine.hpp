#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

namespace graphics::input {
template <typename T>
concept Hashable = requires(T a) {
    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
    { a == a } -> std::convertible_to<bool>;
};
template <Hashable BUTTON>
class InputEngine {
    public:
        InputEngine(const InputEngine &) = delete;
        InputEngine(InputEngine &&) = delete;
        auto operator=(const InputEngine &) -> InputEngine & = delete;
        auto operator=(InputEngine &&) -> InputEngine & = delete;
        explicit InputEngine(std::string_view engine_name) : engine_name_(engine_name) {}
        virtual ~InputEngine() = default;

        [[nodiscard]] auto GetEngineName() const -> std::string_view { return engine_name_; }
        void ResetButtonState() {
            std::scoped_lock lock(mutex_);
            for (auto &[k, v] : button_states_) {
                button_states_[k] = false;
            }
        }

        [[nodiscard]] auto IsPressed(BUTTON button) const -> bool {
            std::scoped_lock lock(mutex_);
            auto it = button_states_.find(button);
            if (it != button_states_.end()) {
                return it->second;
            }
            return false;
        }

    protected:
        void SetButtonState(BUTTON button, bool is_pressed) {
            std::scoped_lock lock(mutex_);
            button_states_[button] = is_pressed;
        }
        mutable std::mutex mutex_;

    private:
        std::string engine_name_;
        std::unordered_map<BUTTON, bool> button_states_;
};

}  // namespace graphics::input