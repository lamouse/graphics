#ifndef G_APP_HPP
#define G_APP_HPP
#include "g_window.hpp"
#include "g_game_object.hpp"
#include "g_render.hpp"
#include "resource/image_texture.hpp"
#include "g_descriptor.hpp"
#include <iostream>
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
            GameObject::Map gameObjects;
            void loadGameObjects();
            ::std::unique_ptr<DescriptorPool> descriptorPool_;
    };
}

#endif