#include "app.hpp"

#include "graphic.hpp"
#include "resource/mesh_instance.hpp"
#include "ecs/components/camera_component.hpp"
#include "effects/particle/particle.hpp"
#include "effects/light/point_light.hpp"
#include "effects/model/model.hpp"
#include "effects/model/multi_mesh_model.hpp"
#include "effects/cubemap/skybox.hpp"
#include "system/setting_ui.hpp"
#include "world/world.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include "render_core/framebufferConfig.hpp"
#include "gui.hpp"
#include "system/camera_system.hpp"
#include "system/pick_system.hpp"
#include <tracy/Tracy.hpp>
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
    const auto frame_layout = window->getFramebufferLayout();
    core::FrameInfo frameInfo{};
    auto* graphics = render_base->getGraphics();
    ui::MenuData menu_data{};

    ui::StatusBarData statusData;
    auto deviceVendor = render_base->GetDeviceVendor();
    statusData.device_name = deviceVendor;

    render::frame::FramebufferConfig frames{
        .width = frame_layout.width, .height = frame_layout.height, .stride = frame_layout.width};
    bool show_debug_ui = true;
    world::World world;

    auto& cameraComponent =
        world.getEntity(world::WorldEntityType::CAMERA).getComponent<ecs::CameraComponent>();
    auto camera = cameraComponent.getCamera();

    float current_mouse_X = 0, current_mouse_Y = 0;
    render::CleanValue frameClean{};
    frameClean.width = frame_layout.width;
    frameClean.hight = frame_layout.height;
    frameClean.framebuffer.color_formats.at(0) = render::surface::PixelFormat::B8G8R8A8_UNORM;
    frameClean.framebuffer.depth_format = render::surface::PixelFormat::D32_FLOAT;
    frameClean.framebuffer.extent = {
        .width = frame_layout.width, .height = frame_layout.height, .depth = 1};
    while (!window->shouldClose()) {
        if (window->IsMinimized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        graphics->clean(frameClean);
        outliner_entities_ = registry.getOutliner();
        outliner_entities_.push_back({.entity = world.entity_, .children = world.getEntities()});
        window->pullEvents(input_event);
        cameraComponent.setAspect(window->getAspectRatio());
        camera = cameraComponent.getCamera();
        frameInfo.camera = &camera;
        auto [duration, frame] = getRuntime();
        frameInfo.frameTime = frame;
        frameInfo.durationTime = duration;
        frameInfo.resource_manager = &resourceManager;
        frameInfo.window_width = window->getActiveConfig().extent.width;
        frameInfo.window_hight = window->getActiveConfig().extent.height;
        bool is_no_input = true;
        while (auto e = input_event.pop_event()) {
            if (!e) {
                continue;
            }

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
            if (!e->onlyMouseMove()) {
                const auto [down, first] = e->mouseLeftButtonDown();
                if (down && first) {
                    auto pick = PickingSystem::pick(camera, e->mouseX_, e->mouseY_,
                                                    frameInfo.window_width, frameInfo.window_hight);
                    if (pick) {
                        spdlog::debug("pick id{}", pick->id);
                        world.pick(pick->id);
                    }
                }
                if (e->mouseLeftButtonUp()) {
                    world.cancelPick();
                }
                CameraSystem::update(cameraComponent, e.value(), frame);
                frameInfo.input_state = e.value();
                registry.updateAll(frameInfo, world);
                is_no_input = false;
            }
        }
        if (is_no_input) {
            registry.updateAll(frameInfo, world);
        }
        registry.drawAll(graphics);

        auto& shader_notify = render_base->getShaderNotify();
        const int shaders_building = shader_notify.ShadersBuilding();

        if (show_debug_ui) {
            if (shaders_building > 0) {
                statusData.build_shaders = shaders_building;
            } else {
                statusData.build_shaders = 0;
            }
            statusData.registry_count = static_cast<int>(registry.size());
            statusData.mouseX_ = current_mouse_X;
            statusData.mouseY_ = current_mouse_Y;
            auto imageId = graphics->getDrawImage();
            auto ui_fun = [&]() -> void {
                ui::show_menu(menu_data);
                draw_setting(menu_data.show_system_setting);
                ui::ShowOutliner(outliner_entities_, menu_data);
                render_status_bar(menu_data, statusData);
                ui::draw_texture(menu_data, imageId, window->getAspectRatio());
                logger.drawUi(menu_data.show_log);
            };
            render_base->composite(std::span{&frames, 1}, ui_fun);
        } else {
            render_base->composite(std::span{&frames, 1});
        }

        frameInfo.clean();
        world.cleanLight();
    }
}

App::App()
    : window(createWindow()),
      render_base(createRender(window.get())),
      resourceManager(render_base->getGraphics()) {}

App::~App() = default;

void App::load_resource() {
    std::string viking_obj_path = "backpack";
    std::string model_shader_name = "model";
    std::string particle_shader_name = "particle";
    std::string point_light_shader_name = "point_light";

    resourceManager.addGraphShader(model_shader_name);
    resourceManager.addGraphShader(particle_shader_name);
    resourceManager.addGraphShader(point_light_shader_name);
    resourceManager.addComputeShader(particle_shader_name);
    auto frame_layout = window->getFramebufferLayout();

    ModelResourceName names{.shader_name = model_shader_name, .mesh_name = viking_obj_path};

    std::array light_colors = {glm::vec3{1.f, 0.f, 0.f}, glm::vec3{0.f, 1.f, 0.f},
                               glm::vec3{0.f, 0.f, 1.f}, glm::vec3{1.f, 1.f, 0.f},
                               glm::vec3{1.f, 0.f, 1.f}, glm::vec3{0.f, 1.f, 1.f},
                               glm::vec3{1.f, 1.f, 1.f}};
    for (auto& light_color : light_colors) {
        auto point_light = std::make_shared<effects::PointLightEffect>(
            resourceManager, frame_layout, 1, .04f, light_color);
        registry.add(point_light);
    }

    // auto delta_particle =
    //     std::make_shared<effects::DeltaParticle>(resourceManager, frame_layout, PARTICLE_COUNT);
    // registry.add(delta_particle);
    auto light_model =
        std::make_shared<effects::LightModel>(resourceManager, frame_layout, names, "model");
    registry.add(light_model);
    auto sky_box = std::make_shared<effects::SkyBox>(resourceManager, frame_layout);
    registry.add(sky_box);
}

}  // namespace graphics
