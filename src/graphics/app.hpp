#pragma once
#include "../config.hpp"
#include "core/frontend/window.hpp"
#include "system/system_config.hpp"
#include <memory>
namespace graphics {
class App {
    public:
        void run();
        explicit App(const g::Config &config);
        App(const App &) = delete;
        App(App &&) = delete;
        auto operator=(const App &) -> App & = delete;
        auto operator=(App &&) -> App & = delete;
        ~App();

    private:
        std::unique_ptr<core::frontend::BaseWindow> window;
        config::ImageQuality imageQualityConfig;
};
}  // namespace g
