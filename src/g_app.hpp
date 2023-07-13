#pragma once
#include "g_descriptor.hpp"
#include "g_game_object.hpp"
#include "g_window.hpp"


namespace g {
class App {
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
}  // namespace g
