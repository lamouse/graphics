#pragma once
#include <memory>

#include "resource/resource.hpp"
#include "render_core/render_base.hpp"
#include "common/common_funcs.hpp"
#include "system/logger_system.hpp"
#include "world/world.hpp"



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
        std::unique_ptr<core::frontend::BaseWindow> window;
        std::unique_ptr<render::RenderBase> render_base;
        ResourceManager resourceManager;
        core::InputEvent input_event;
        world::World world;
        void load_resource();
        std::shared_ptr<input::InputSystem> input_system_;
};
}  // namespace graphics
