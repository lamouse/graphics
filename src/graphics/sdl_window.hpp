#pragma once
#include "core/frontend/window.hpp"
#include <SDL3/SDL.h>
#include <memory>
namespace graphics {
namespace input {
class InputSystem;

}
class SDLWindow : public core::frontend::BaseWindow {
    public:
        SDLWindow(std::shared_ptr<input::InputSystem> input_system, int width, int height,
                  ::std::string_view title);
        SDLWindow(const SDLWindow&) = delete;
        SDLWindow(SDLWindow&&) noexcept = delete;
        auto operator=(const SDLWindow&) -> SDLWindow& = delete;
        auto operator=(SDLWindow&&) noexcept -> SDLWindow& = delete;
        ~SDLWindow() override;
        [[nodiscard]] auto IsShown() const -> bool override;
        [[nodiscard]] auto IsMinimized() const -> bool override;
        [[nodiscard]] auto shouldClose() const -> bool override;
        void OnFrameDisplayed() override {};
        [[nodiscard]] auto getActiveConfig() const -> WindowConfig override {
            return getWindowConfig();
        }
        void setWindowTitle(std::string_view title) override;
        void configGUI() override;
        void destroyGUI() override;
        void newFrame() override;
        void pullEvents() override;
        void setShouldClose() override { should_close_ = true; };
        auto getWindow() -> SDL_Window* { return window_; }

    private:
        SDL_Window* window_ = nullptr;
        bool should_close_ = false;
        std::shared_ptr<input::InputSystem> input_system_;
};
}  // namespace graphics