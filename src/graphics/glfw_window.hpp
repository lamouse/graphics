#pragma once
#include <GLFW/glfw3.h>

#include <string>

#include "core/frontend/window.hpp"
namespace graphics {
class ScreenWindow : public core::frontend::BaseWindow {
    private:
        GLFWwindow *window = nullptr;
        void initWindow();
        ::std::string title_;

    public:
        ScreenWindow(int width, int height, ::std::string title);
        ScreenWindow(const ScreenWindow &) = delete;
        auto operator=(const ScreenWindow &) -> ScreenWindow & = delete;
        ScreenWindow(ScreenWindow &&) noexcept;
        auto operator=(ScreenWindow &&) noexcept -> ScreenWindow &;
        auto operator()() -> GLFWwindow * { return window; }
        ScreenWindow() = default;
        ~ScreenWindow() override;
        [[nodiscard]] auto IsShown() const -> bool override;
        [[nodiscard]] auto IsMinimized() const -> bool override;
        [[nodiscard]] auto shouldClose() const -> bool override;
        void OnFrameDisplayed() override {};
        [[nodiscard]] auto getActiveConfig() const -> WindowConfig override {
            return getWindowConfig();
        }
        void setWindowTitle(std::string_view title) override {
            glfwSetWindowTitle(window, title.data());
        };
        void configGUI() override;
        void destroyGUI() override;
        void newFrame() override;
        void pullEvents(core::InputEvent &event) override { glfwPollEvents(); }
        void setShouldClose() override { glfwSetWindowShouldClose(window, true); };
        [[nodiscard]] auto getScale() const -> float { return window_info.render_surface_scale; };
};
}  // namespace graphics