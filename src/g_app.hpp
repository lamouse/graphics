#pragma once
#include "g_descriptor.hpp"
#include "g_game_object.hpp"
#include "g_window.hpp"
#include "system/system_config.hpp"
#include "core/buffer.hpp"


namespace g {
class App {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;
        void run();
        App();
        App(const App &) = delete;
        App(App &&) = delete;
        auto operator=(const App &) -> App & = delete;
        auto operator=(App &&) -> App & = delete;
        ~App() = default;

    private:
        /**
         * @brief Device extensions
         *
         */
        ::std::vector<const char *> deviceExtensions = {
#if defined(VK_USE_PLATFORM_MACOS_MVK)
            "VK_KHR_portability_subset",  // "VK_KHR_portability_subset" macos
#endif
            VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        Window window{{WIDTH, HEIGHT}, "vulkan"};
#ifdef NO_DEBUG
        static constexpr bool enableValidationLayers = false;
#else
        static constexpr bool enableValidationLayers = true;
#endif
        core::Device device_{Window::getRequiredInstanceExtends(enableValidationLayers), deviceExtensions,
                             [=, this](VkInstance instance) -> VkSurfaceKHR { return window.getSurface(instance); },
                             enableValidationLayers};
        config::ImageQuality imageQualityConfig;
        GameObject::Map gameObjects;
        void loadGameObjects();
        ::std::unique_ptr<DescriptorPool> descriptorPool_;
};
}  // namespace g
