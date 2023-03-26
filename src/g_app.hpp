#ifndef G_APP_HPP
#define G_APP_HPP
#include "g_window.hpp"
#include "g_game_object.hpp"
#include "g_render.hpp"
#include<iostream>
#include <vector>

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
            ::std::vector<GameObject> gameObjects;
            RenderProcesser render;
            void loadGameObjects();
    };
}

#endif