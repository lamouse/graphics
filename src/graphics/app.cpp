#include "app.hpp"

#include "graphic.hpp"
#include "resource/model_instance.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/components/camera_component.hpp"
#include "effects/particle/particle.hpp"
#include "effects/light/point_light.hpp"
#include "effects/model/model.hpp"
#include "system/setting_ui.hpp"
#include "world/world.hpp"
#include <spdlog/spdlog.h>
#include "render_core/render_vulkan/render_vulkan.hpp"
#include <thread>
#include "render_core/framebufferConfig.hpp"
#include "gui.hpp"
#define image_path ::std::string{"./images/"}
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
    load_resource();
    std::vector<ecs::Entity> model_entt;
    std::vector<effects::LightModel> models;
    core::FrameInfo frameInfo{};
    auto* graphics = render_base->getGraphics();
    ui::MenuData menu_data{};
    render::frame::FramebufferConfig frames{.width = 1920, .height = 1080, .stride = 1920};
    render::PipelineState pipeline_state;
    auto layout = window->getFramebufferLayout();
    pipeline_state.viewport.width = layout.screen.GetWidth();
    pipeline_state.viewport.height = layout.screen.GetHeight();
    pipeline_state.scissors.width = layout.screen.GetWidth();
    pipeline_state.scissors.height = layout.screen.GetHeight();
    std::string model_shader_name = "model";
    std::string particle_shader_name = "particle";

    world::World world;
    [[maybe_unused]] bool show_console_logger = false;
    std::string viking_room_path = image_path + "viking_room.png";
    std::string other_image = image_path + "p1.jpg";
    std::string viking_obj_path = "models/viking_room.obj";
    effects::PointLightEffect pointLight{resourceManager};
    effects::DeltaParticle particle(resourceManager, graphics, PARTICLE_COUNT);
    ModelResourceName names{.shader_name = model_shader_name,
                            .mesh_name = viking_obj_path,
                            .texture_name = viking_room_path};
    models.emplace_back(resourceManager, names, "model");
    model_entt.emplace_back(particle.entity_);
    auto particle_child = particle.getChildEntitys();
    model_entt.insert(model_entt.end(), particle_child.begin(), particle_child.end());
    model_entt.emplace_back(pointLight.entity_);
    auto pointLight_child = pointLight.getChildEntitys();
    model_entt.insert(model_entt.end(), pointLight_child.begin(), pointLight_child.end());
    for (const auto& model : models) {
        model_entt.push_back(model.entity_);
        auto model_child = model.getChildEntitys();
        model_entt.insert(model_entt.end(), model_child.begin(), model_child.end());
    }
    model_entt.push_back(world.getEntity(world::WorldEntityType::CAMERA));

    auto& cameraComponent =
        world.getEntity(world::WorldEntityType::CAMERA).getComponent<ecs::CameraComponent>();
    auto camera = cameraComponent.getCamera();
    while (!window->shouldClose()) {
        camera = cameraComponent.getCamera();
        window->pullEvents();
        if (window->IsMinimized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (cameraComponent.extentAspectRation != window->getAspectRatio()) {
            cameraComponent.extentAspectRation = window->getAspectRatio();
        }
        frameInfo.frameTime = getRuntime();
        frameInfo.camera = &camera;

        for (auto& m : models) {
            graphics->setPipelineState(pipeline_state);
            m.update(frameInfo);
            m.draw(graphics);
        }
        particle.getUniforBuffer().deltaTime = frameInfo.frameTime;
        graphics->setPipelineState(pipeline_state);
        particle.draw(graphics);
        camera.setPerspectiveProjection(glm::radians(50.f), window->getAspectRatio(), 0.1f, 100.f);
        pointLight.update(frameInfo);
        graphics->setPipelineState(pipeline_state);
        pointLight.draw(graphics);
        auto& shader_notify = render_base->getShaderNotify();
        const int shaders_building = shader_notify.ShadersBuilding();
        if (shaders_building > 0) {
            window->setWindowTitle(fmt::format("Building {} shader(s)", shaders_building));
        } else {
            window->setWindowTitle("graphics");
        }
        auto imageId = graphics->getDrawImage();
        render_base->addImguiUI([&]() {
            ui::show_menu(menu_data);
            draw_setting(menu_data.show_system_setting);
            ui::ShowOutliner(model_entt, menu_data);
            ui::draw_result(menu_data, imageId, window->getAspectRatio());
            ui::pipeline_state(pipeline_state);
            logger.drawUi(menu_data.show_log);
        });
        render_base->composite(std::span{&frames, 1});
        graphics->clean();
    }
}

App::App()
    : window(createWindow()),
      render_base(createRender(window.get())),
      resourceManager(render_base->getGraphics()) {}

App::~App() = default;

void App::load_resource() {
    std::string viking_room_path = image_path + "viking_room.png";
    std::string other_image = image_path + "p1.jpg";
    resourceManager.addTexture(viking_room_path);
    resourceManager.addTexture(other_image);

    std::string viking_obj_path = "models/viking_room.obj";
    resourceManager.addMesh(viking_obj_path);
    std::string model_shader_name = "model";
    std::string particle_shader_name = "particle";
    std::string point_light_shader_name = "point_light";

    resourceManager.addGraphShader(model_shader_name);
    resourceManager.addGraphShader(particle_shader_name);
    resourceManager.addGraphShader(point_light_shader_name);
    resourceManager.addComputeShader(particle_shader_name);
}

}  // namespace graphics
