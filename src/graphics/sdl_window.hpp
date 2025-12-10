#pragma once
#include "core/frontend/window.hpp"
#include "common/common_funcs.hpp"
#include <SDL3/SDL.h>
namespace graphics {
class SDLWindow : public core::frontend::BaseWindow {
    public:
        SDLWindow(int width, int height, ::std::string_view title);
        CLASS_NON_COPYABLE(SDLWindow);
        CLASS_NON_MOVEABLE(SDLWindow);
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
        void pullEvents(core::InputEvent& event) override;
        void setShouldClose() override { should_close_ = true; };
        auto getWindow() -> SDL_Window* { return window_; }

    private:
        SDL_Window* window_ = nullptr;
        bool should_close_ = false;
        SDL_GPUDevice* gpu_device;
};
}  // namespace graphics