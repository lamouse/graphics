#pragma once
#include <memory>

#include "../config.hpp"
#include "render_core/render_base.hpp"
#include "system/system_config.hpp"
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
        std::unique_ptr<render::RenderBase> render_base;
        config::ImageQuality imageQualityConfig;
};
}  // namespace graphics
