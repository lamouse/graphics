#ifndef G_APP_HPP
#define G_APP_HPP
#include "g_window.hpp"
#include "g_pipline.hpp"
#include <filesystem>
#include<iostream>

namespace g
{
    class App
    {
        public:
            static constexpr int WIDTH = 800;
            static constexpr int HEIGHT = 600;
            void run();
            App();
            ~App();
        private:
            Window window{WIDTH, HEIGHT, "vulkan"};
    };
}

#endif