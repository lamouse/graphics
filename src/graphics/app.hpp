#pragma once
#include <memory>

#include "resource/resource.hpp"
#include "render_core/render_base.hpp"
#include "render_core/framebufferConfig.hpp"
#include "common/common_funcs.hpp"
#include "system/logger_system.hpp"
#include "world/world.hpp"
#include "QT_window.hpp"
#include "graphics/gui.hpp"

namespace graphics {
namespace input {
class InputSystem;
}
class RenderRegistry;
class App {
    public:
        void run();
        explicit App();
        CLASS_NON_COPYABLE(App);
        CLASS_NON_MOVEABLE(App);
        ~App();

    private:
        sys::LoggerSystem logger;
        std::shared_ptr<input::InputSystem> input_system_;
        std::unique_ptr<QTWindow> qt_main_window;
        core::frontend::BaseWindow* window;

        void render();

        std::unique_ptr<render::RenderBase> render_base;
        std::unique_ptr<ResourceManager> resourceManager;
        core::InputEvent input_event;
        world::World world;

        render::frame::FramebufferConfig frame_config_;
        render::CleanValue frameClean{};
        ui::StatusBarData statusData;
        void load_resource();
};
}  // namespace graphics
