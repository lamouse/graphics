#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include "g_defines.hpp"
#include "core/frontend/window.hpp"
namespace g {
class Window : public core::frontend::BaseWindow{
    private:
        GLFWwindow *window;
        void initWindow();
        ::std::string title_;

    public:
        Window(ScreenExtent extent, ::std::string title);
        Window(const Window &) = delete;
        auto operator=(const Window &) -> Window & = delete;
        Window(Window &&) noexcept;
        auto operator=(Window &&) noexcept -> Window &;
        auto operator()() -> GLFWwindow * { return window; }
        Window() = default;
        ~Window() override;
        [[nodiscard]] auto IsShown() const -> bool override;
        [[nodiscard]] auto IsMinimized() const -> bool override;
        [[nodiscard]] auto shouldClose() const -> bool override;
        [[nodiscard]] auto getActiveConfig() const -> WindowConfig override{ return getWindowConfig(); }
        [[nodiscard]] auto getScale() const -> float { return window_info.render_surface_scale; };
        static auto getRequiredInstanceExtends(bool enableValidationLayers) -> ::std::vector<const char *>;
        auto getSurface(VkInstance instance) -> VkSurfaceKHR;
        auto getExtent() -> ScreenExtent;
};
}  // namespace g