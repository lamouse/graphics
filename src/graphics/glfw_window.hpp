#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include "core/frontend/window.hpp"
#include "g_defines.hpp"
namespace graphics {
class ScreenWindow : public core::frontend::BaseWindow {
    private:
        GLFWwindow *window;
        void initWindow();
        ::std::string title_;

    public:
        ScreenWindow(ScreenExtent extent, ::std::string title);
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
        void pullEvents() override { glfwPollEvents(); }
        [[nodiscard]] auto getScale() const -> float { return window_info.render_surface_scale; };
        static auto getRequiredInstanceExtends(bool enableValidationLayers)
            -> ::std::vector<const char *>;
        auto getSurface(VkInstance instance) -> VkSurfaceKHR;
        auto getExtent() -> ScreenExtent;
};
}  // namespace graphics