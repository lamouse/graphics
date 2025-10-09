#include "app.hpp"

#include "config/window.h"
#include "resource/model_instance.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/components/camera_component.hpp"
#include "world/world.hpp"
#include <spdlog/spdlog.h>
#include "render_core/render_vulkan/render_vulkan.hpp"
#if defined(USE_GLFW)
#include "glfw_window.hpp"
#endif
#include "sdl_window.hpp"
#include <thread>
#include "render_core/framebufferConfig.hpp"
#include "ui.hpp"
#include "ui/ui.hpp"
#define image_path ::std::string{"./images/"}
#define shader_path ::std::string{"./shader/"}
#define models_path ::std::string{"./models/"}

namespace graphics {
namespace {
auto getRuntime() -> float {
    static auto startTime = ::std::chrono::high_resolution_clock::now();
    auto currentTime = ::std::chrono::high_resolution_clock::now();
    return ::std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime)
        .count();
}
}  // namespace

void App::run() {
    auto* graphics = render_base->getGraphics();

    render::frame::FramebufferConfig frames{.width = 1920, .height = 1080, .stride = 1920};
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    render::PipelineState pipeline_state;
    auto layout = window->getFramebufferLayout();
    pipeline_state.viewport.width = layout.screen.GetWidth();
    pipeline_state.viewport.height = layout.screen.GetHeight();
    pipeline_state.scissors.width = layout.screen.GetWidth();
    pipeline_state.scissors.height = layout.screen.GetHeight();

    world::World world;
    bool show_console_logger = false;
    ModelInstance modelInstance = ModelInstance::createGameObject(image_path + "viking_room.png", "models/viking_room.obj", sizeof(UniformBufferObject));
     auto graphicId = graphics->uploadModel(modelInstance);
     modelInstance.setModelId(graphicId);
    auto& camera =
        world.getEntity(world::WorldEntityType::CAMERA).getComponent<ecs::CameraComponent>();
    auto& modelComponent = modelInstance.getEntity().getComponent<ecs::TransformComponent>();
    while (!window->shouldClose()) {
        window->pullEvents();
        if (window->IsMinimized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (camera.extentAspectRation != window->getAspectRatio()) {
            camera.extentAspectRation = window->getAspectRatio();
        }
        modelComponent.rotation.z = getRuntime() * glm::radians(90.0F);
        UniformBufferObject ubo1{};
        UniformBufferObject ubo2{};
        ubo1.model = modelInstance.getModelMatrix();
        ubo1.view = camera.getCamera().getView();
        ubo1.proj = camera.getCamera().getProjection();
        modelInstance.writeToUBOMapData(ubo1.as_byte_span());
        graphics->setPipelineState(pipeline_state);
        graphics->draw(modelInstance);
        ubo2 = ubo1;
        auto model2 = modelComponent;
        model2.translation.x = modelComponent.translation.x + 1.8F;
        model2.scale.x = modelComponent.scale.x * 0.5F;
        model2.scale.y = modelComponent.scale.y * 0.5F;
        model2.scale.z = modelComponent.scale.z * 0.5F;
        ubo2.model = model2.mat4();
        modelInstance.writeToUBOMapData(ubo2.as_byte_span());
        graphics->draw(modelInstance);
        graphics->end();
        auto& shader_notify = render_base->getShaderNotify();
        const int shaders_building = shader_notify.ShadersBuilding();
        if (shaders_building > 0) {
            window->setWindowTitle(fmt::format("Building {} shader(s)", shaders_building));
        } else {
            window->setWindowTitle("graphics");
        }
#if defined(USE_DEBUG_UI)
        auto imageId = graphics->getDrawImage();
        render_base->addImguiUI([&]() {
            graphics::ui::begin();
            //graphics::ui::draw_result(imageId, window->getAspectRatio());
            graphics::ui::pipeline_state(pipeline_state);
            graphics::ui::draw_setting();
            logger.drawUi(show_console_logger);
            world.drawUI();
            modelInstance.drawUI();
            graphics::ui::end();
        });
#endif
        render_base->composite(std::span{&frames, 1});
        graphics->clean();
    }
}

App::App(const g::Config& config):logger(config) {
    auto [width, height, title] = config.getConfig<config::window::Window>();

#if defined(USE_GLFW)
    window = std::make_unique<ScreenWindow>(ScreenExtent{.width = width, .height = height}, title);
#endif
#if defined(USE_SDL)
    window = std::make_unique<graphics::SDLWindow>(width, height, title);
#endif
    render_base = std::make_unique<render::vulkan::RendererVulkan>(window.get());
}

App::~App() {};

}  // namespace graphics
