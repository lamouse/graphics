#pragma once
#include "g_window.hpp"
#include "system/system_config.hpp"

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
        ~App();

    private:
        /**
         * @brief Device extensions
         *
         */
        ::std::vector<const char *> deviceExtensions = {
#if defined(VK_USE_PLATFORM_MACOS_MVK)
            "VK_KHR_portability_subset",  // "VK_KHR_portability_subset" macos
#endif
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            // VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME
        };
        Window window{{WIDTH, HEIGHT}, "vulkan"};
        config::ImageQuality imageQualityConfig;
};
}  // namespace g
