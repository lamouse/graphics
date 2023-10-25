#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include "g_defines.hpp"

namespace g {
class Window {
    private:
        GLFWwindow *window;
        int width;
        int height;
        float scale;
        void initWindow();
        ::std::string title_;

    public:
        Window(ScreenExtent extent, ::std::string title);
        Window(const Window &) = delete;
        auto operator=(const Window &) -> Window & = delete;
        Window(Window &&) = default;
        auto operator=(Window &&) -> Window & = default;
        auto operator()() -> GLFWwindow * { return window; }
        ~Window();
        auto shouldClose() -> bool;
        [[nodiscard]] auto getScale() const -> float { return scale; };
        static auto getRequiredInstanceExtends(bool enableValidationLayers) -> ::std::vector<const char *>;
        auto getSurface(VkInstance instance) -> VkSurfaceKHR;
        auto getExtent() -> ScreenExtent;
};
}  // namespace g