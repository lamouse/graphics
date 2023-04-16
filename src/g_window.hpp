#ifndef G_WINDOW_HPP
#define G_WINDOW_HPP
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
            void initWindow();
            ::std::string title;
        public:
            Window(int weidth, int height, ::std::string title);
            Window(const Window&) = delete;
            Window &operator=(const Window&) = delete;
            GLFWwindow *operator()(){return window;}
            ~Window();
            bool shuldClose();
    };
}

#endif