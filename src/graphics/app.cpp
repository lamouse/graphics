#include "app.hpp"

#include "graphic.hpp"
#include "resource/model_instance.hpp"
#include "ecs/components/camera_component.hpp"
#include "effects/particle/particle.hpp"
#include "effects/light/point_light.hpp"
#include "effects/model/model.hpp"
#include "effects/cubemap/skybox.hpp"
#include "system/setting_ui.hpp"
#include "world/world.hpp"
#include <spdlog/spdlog.h>
#include "render_core/render_vulkan/render_vulkan.hpp"
#include <thread>
#include "render_core/framebufferConfig.hpp"
#include "gui.hpp"
#include "system/camera_system.hpp"
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
    std::vector<effects::LightModel> models;
    const auto frame_layout = window->getFramebufferLayout();
    core::FrameInfo frameInfo{};
    auto* graphics = render_base->getGraphics();
    ui::MenuData menu_data{};
    render::frame::FramebufferConfig frames{
        .width = frame_layout.width, .height = frame_layout.height, .stride = frame_layout.width};
    bool show_debug_ui = true;
    world::World world;
    model_entt.push_back(world.getEntity(world::WorldEntityType::CAMERA));
    auto& cameraComponent =
        world.getEntity(world::WorldEntityType::CAMERA).getComponent<ecs::CameraComponent>();
    auto camera = cameraComponent.getCamera();
    model_entt.insert(model_entt.end(), registry.getEntt().begin(), registry.getEntt().end());
    float current_mouse_X = 0, current_mouse_Y = 0;
    const float speed = 5.0f;
    while (!window->shouldClose()) {
        if (window->IsMinimized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        window->pullEvents(input_event);
        while (auto e = input_event.pop_event()) {
            // if(show_debug_ui && ui::IsMouseControlledByImGui() && (e->mouse_button_left.Value() || e->mouse_button_right.Value())){
            //     continue;
            // }
            glm::vec3 moveDir(0.0f);
            switch (e->key) {
                case core::InputKey::Insert: {
                    show_debug_ui = !show_debug_ui;
                    break;
                }
            }
            if (e->mouseX_ > 0 && e->mouseY_ > 0) {
                current_mouse_X = e->mouseX_;
                current_mouse_Y = e->mouseY_;
            }
            CameraSystem::update(cameraComponent, e.value());
        }
        cameraComponent.extentAspectRation = window->getAspectRatio();
        camera = cameraComponent.getCamera();
        frameInfo.camera = &camera;
        frameInfo.frameTime = getRuntime();

        registry.updateAll(frameInfo);
        registry.drawAll(graphics);

        auto& shader_notify = render_base->getShaderNotify();
        const int shaders_building = shader_notify.ShadersBuilding();
        if (shaders_building > 0) {
            window->setWindowTitle(fmt::format("Building {} shader(s)", shaders_building));
        } else {
            window->setWindowTitle("graphics");
        }
        auto imageId = graphics->getDrawImage();
        if (show_debug_ui) {
            render_base->addImguiUI([&]() -> void {
                ui::show_menu(menu_data);
                draw_setting(menu_data.show_system_setting);
                ui::ShowOutliner(model_entt, menu_data);
                render_status_bar(menu_data, current_mouse_X, current_mouse_Y,
                                  static_cast<int>(registry.size()));
                ui::draw_texture(menu_data, imageId, window->getAspectRatio());
                logger.drawUi(menu_data.show_log);
            });
        }

        render_base->composite(std::span{&frames, 1});
        // TODO 添加clear value
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
    resourceManager.addTexture(viking_room_path);

    std::string viking_obj_path = "models/viking_room.obj";
    resourceManager.addMesh(viking_obj_path);
    std::string model_shader_name = "model";
    std::string particle_shader_name = "particle";
    std::string point_light_shader_name = "point_light";

    resourceManager.addGraphShader(model_shader_name);
    resourceManager.addGraphShader(particle_shader_name);
    resourceManager.addGraphShader(point_light_shader_name);
    resourceManager.addComputeShader(particle_shader_name);
    auto frame_layout = window->getFramebufferLayout();
    auto sky_box = std::make_shared<effects::SkyBox>(resourceManager, frame_layout);
    registry.add(sky_box);

    ModelResourceName names{.shader_name = model_shader_name,
                            .mesh_name = viking_obj_path,
                            .texture_name = viking_room_path};

    auto light_model =
        std::make_shared<effects::LightModel>(resourceManager, frame_layout, names, "model");
    registry.add(light_model);

    auto point_light = std::make_shared<effects::PointLightEffect>(resourceManager, frame_layout);
    registry.add(point_light);

    auto delta_particle =
        std::make_shared<effects::DeltaParticle>(resourceManager, frame_layout, PARTICLE_COUNT);
    registry.add(delta_particle);
}

}  // namespace graphics
