#pragma once
#include <memory>

#include "resource/config.hpp"
#include "render_core/render_base.hpp"
#include "common/common_funcs.hpp"
#include "system/logger_system.hpp"
namespace graphics {
class App {
    public:
        void run();
        explicit App(const g::Config &config);
        CLASS_NON_COPYABLE(App);
        CLASS_NON_MOVEABLE(App);
        ~App();

    private:
        std::unique_ptr<core::frontend::BaseWindow> window;
        std::unique_ptr<render::RenderBase> render_base;
        sys::LoggerSystem logger;
};
}  // namespace graphics
