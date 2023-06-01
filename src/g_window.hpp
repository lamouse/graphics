#pragma once
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include <string>
#include <vector>

namespace g{
    class Window
    {
        private:
            GLFWwindow *window;
            int width;
            int height;
            float scale;
            void initWindow();
            ::std::string title;
        public:
            Window(int width, int height, ::std::string title);
            Window(const Window&) = delete;
            auto operator=(const Window&) -> Window & = delete;
            auto operator()() -> GLFWwindow *{return window;}
            ~Window();
            auto shuldClose() -> bool;
            [[nodiscard]] auto getScale() const -> float{return scale;};
    };
}