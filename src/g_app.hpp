#pragma once
#include "config.hpp"
#include "g_window.hpp"
#include "system/system_config.hpp"

namespace g {
class App {
    public:
        void run();
        explicit App(const Config &config);
        App(const App &) = delete;
        App(App &&) = delete;
        auto operator=(const App &) -> App & = delete;
        auto operator=(App &&) -> App & = delete;
        ~App();

    private:
        Window window;
        config::ImageQuality imageQualityConfig;
};
}  // namespace g
