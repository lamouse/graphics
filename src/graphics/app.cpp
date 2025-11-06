#include "app.hpp"

#include "graphic.hpp"
#include "resource/mesh_instance.hpp"
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
struct FrameTime {
        float duration;
        float frame;
};
auto getRuntime() -> FrameTime {
    FrameTime frameTIme{};
    static auto startTime = ::std::chrono::high_resolution_clock::now();
    static auto lastTime = startTime;
    auto currentTime = ::std::chrono::high_resolution_clock::now();
    frameTIme.duration =
        ::std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime)
            .count();
    frameTIme.frame =
        ::std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime)
            .count();
    lastTime = currentTime;
    return frameTIme;
}
}  // namespace

void App::run() {
    load_resource();
    std::vector<effects::LightModel> models;
    const auto frame_layout = window->getFramebufferLayout();
    core::FrameInfo frameInfo{};
    frameInfo.frame_layout = frame_layout;
    auto* graphics = render_base->getGraphics();
    ui::MenuData menu_data{};

    ui::StatusBarData statusData;
    auto deviceVendor = render_base->GetDeviceVendor();
    statusData.device_name = deviceVendor;

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
    while (!window->shouldClose()) {
        if (window->IsMinimized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        window->pullEvents(input_event);
        cameraComponent.setAspect(window->getAspectRatio());
        camera = cameraComponent.getCamera();
        frameInfo.camera = &camera;
        auto [duration, frame] = getRuntime();
        frameInfo.frameTime = duration;
        frameInfo.resource_manager = &resourceManager;
        if (input_event.empty()) {
            registry.updateAll(frameInfo);
        }
        while (auto e = input_event.pop_event()) {
            if (e->key == core::InputKey::Insert) {
                show_debug_ui = !show_debug_ui;
            }
            if (e->key == core::InputKey::Esc) {
                window->setShouldClose();
            }
            if (e->mouseX_ > 0 && e->mouseY_ > 0) {
                current_mouse_X = e->mouseX_;
                current_mouse_Y = e->mouseY_;
            }
            CameraSystem::update(cameraComponent, e.value(), frame);
            frameInfo.input_state = e.value();
            registry.updateAll(frameInfo);
        }

        registry.drawAll(graphics);

        auto& shader_notify = render_base->getShaderNotify();
        const int shaders_building = shader_notify.ShadersBuilding();

        auto imageId = graphics->getDrawImage();

        if (show_debug_ui) {
            if (shaders_building > 0) {
                statusData.build_shaders = shaders_building;
            } else {
                statusData.build_shaders = 0;
            }
            statusData.registry_count = static_cast<int>(registry.size());
            statusData.mouseX_ = current_mouse_X;
            statusData.mouseY_ = current_mouse_Y;

            auto ui_fun = [&]() -> void {
                ui::show_menu(menu_data);
                draw_setting(menu_data.show_system_setting);
                ui::ShowOutliner(model_entt, menu_data);
                render_status_bar(menu_data, statusData);
                ui::draw_texture(menu_data, imageId, window->getAspectRatio());
                logger.drawUi(menu_data.show_log);
            };
            render_base->composite(std::span{&frames, 1}, ui_fun);
        } else {
            render_base->composite(std::span{&frames, 1});
        }

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

    std::string viking_obj_path = "viking_room.obj";
    resourceManager.addModel(viking_obj_path);
    std::string model_shader_name = "model";
    std::string particle_shader_name = "particle";
    std::string point_light_shader_name = "point_light";

    resourceManager.addGraphShader(model_shader_name);
    resourceManager.addGraphShader(particle_shader_name);
    resourceManager.addGraphShader(point_light_shader_name);
    resourceManager.addComputeShader(particle_shader_name);
    auto frame_layout = window->getFramebufferLayout();

    ModelResourceName names{.shader_name = model_shader_name,
                            .mesh_name = viking_obj_path,
                            .texture_name = viking_room_path};
    auto sky_box = std::make_shared<effects::SkyBox>(resourceManager, frame_layout);
    registry.add(sky_box);
    auto light_model =
        std::make_shared<effects::LightModel>(resourceManager, frame_layout, names, "model");
    registry.add(light_model);

    auto point_light = std::make_shared<effects::PointLightEffect>(resourceManager, frame_layout);
    registry.add(point_light);

    // auto delta_particle =
    //     std::make_shared<effects::DeltaParticle>(resourceManager, frame_layout, PARTICLE_COUNT);
    // registry.add(delta_particle);
}

}  // namespace graphics
