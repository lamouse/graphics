
#pragma once
#include <memory>
#include "resource/resource.hpp"
#include "render_core/framebufferConfig.hpp"
#include "common/class_traits.hpp"
#include "system/logger_system.hpp"
#include "world/world.hpp"
#include "QT_window.hpp"
#include "graphics/gui.hpp"
#include <thread>
#include <core/core.hpp>

namespace graphics {
namespace input {
class InputSystem;
}
class RenderRegistry;
class App {
    public:
        explicit App();
        CLASS_NON_COPYABLE(App);
        CLASS_NON_MOVEABLE(App);
        ~App();

    private:
        sys::LoggerSystem logger;
        std::shared_ptr<input::InputSystem> input_system_;
        std::unique_ptr<QTWindow> qt_main_window;

        void render_thread_func(std::stop_token token);
        void render();

        std::unique_ptr<ResourceManager> resourceManager;
        core::frontend::BaseWindow* window{ nullptr };
        world::World world;

        render::frame::FramebufferConfig frame_config_;
        render::CleanValue frameClean{};
        ui::StatusBarData statusData;
        void load_resource();
        std::jthread render_thread;
        std::unique_ptr<core::System> sys_;
};
}  // namespace graphics
