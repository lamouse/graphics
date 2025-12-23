#include "app.hpp"

#include "graphic.hpp"
#include "resource/mesh_instance.hpp"
#include "ecs/components/camera_component.hpp"
#include "effects/particle/particle.hpp"
#include "effects/light/point_light.hpp"
#include "effects/model/model.hpp"
#include "core/core.hpp"
#include "effects/model/multi_mesh_model.hpp"
#include "common/settings.hpp"
#include "effects/cubemap/skybox.hpp"
#include "system/setting_ui.hpp"
#include "world/world.hpp"
#include <qcoreapplication.h>
#include <spdlog/spdlog.h>
#include "render_core/framebufferConfig.hpp"
#include "gui.hpp"
#include "input/input.hpp"
#include "input/mouse.h"
#include "system/pick_system.hpp"
#include <QTimer>

#include <tracy/Tracy.hpp>

namespace graphics {

void App::run() {
    while (!window->shouldClose()) {
        render();
    }
}
void App::render() {

    window->pullEvents();

    auto* graphics = render_base->getGraphics();
    graphics->clean(frameClean);
    world.update(*window, *resourceManager, *input_system_);

    world.draw(graphics);

    auto& shader_notify = render_base->getShaderNotify();
    const int shaders_building = shader_notify.ShadersBuilding();

    if (settings::values.use_debug_ui.GetValue()) {
        if (shaders_building > 0) {
            statusData.build_shaders = shaders_building;
        } else {
            statusData.build_shaders = 0;
        }
        auto mouse_axis = input_system_->GetMouse()->GetAxis();
        if (mouse_axis.x > 0 && mouse_axis.y > 0) {
            statusData.mouseX_ = mouse_axis.x;
            statusData.mouseY_ = mouse_axis.y;
        }

        auto imageId = graphics->getDrawImage();
        auto ui_fun = [&]() -> void {
            ui::show_menu(settings::values.menu_data);
            draw_setting(settings::values.menu_data.show_system_setting);
            ui::showOutliner(world, settings::values.menu_data);
            render_status_bar(settings::values.menu_data, statusData);
            ui::draw_texture(settings::values.menu_data, imageId, window->getAspectRatio());
            logger.drawUi(settings::values.menu_data.show_log);
        };
        render_base->composite(std::span{&frame_config_, 1}, ui_fun);
    } else {
        render_base->composite(std::span{&frame_config_, 1});
    }

    world.cleanLight();
}

App::App()
    : input_system_(std::make_shared<input::InputSystem>()),
      qt_main_window(std::make_unique<QTWindow>(input_system_, 1920, 1080, "graphics")),
      window(qt_main_window->initRenderWindow()),
      render_base(createRender(window)),
      resourceManager(std::make_unique<ResourceManager>(render_base->getGraphics())) {
    frame_config_ = {.width = 1920, .height = 1080, .stride = 1920};
    frameClean.width = frame_config_.width;
    frameClean.hight = frame_config_.height;
    frameClean.framebuffer.color_formats.at(0) = render::surface::PixelFormat::B8G8R8A8_UNORM;
    frameClean.framebuffer.depth_format = render::surface::PixelFormat::D32_FLOAT;
    frameClean.framebuffer.extent = {
        .width = frame_config_.width, .height = frame_config_.height, .depth = 1};
    auto deviceVendor = render_base->GetDeviceVendor();
    statusData.device_name = deviceVendor;
    load_resource();

    run();
}

App::~App() = default;

void App::load_resource() {
    std::string viking_obj_path = "backpack";
    std::string model_shader_name = "model";
    std::string particle_shader = "particle";
    std::string point_light_shader_name = "point_light";

    resourceManager->addGraphShader(model_shader_name);
    resourceManager->addGraphShader(particle_shader);
    resourceManager->addGraphShader(point_light_shader_name);
    resourceManager->addComputeShader(particle_shader);
    auto frame_layout = window->getFramebufferLayout();

    ModelResourceName names{.shader_name = model_shader_name, .mesh_name = viking_obj_path};

    std::array light_colors = {glm::vec3{1.f, 0.f, 0.f}, glm::vec3{0.f, 1.f, 0.f},
                               glm::vec3{0.f, 0.f, 1.f}, glm::vec3{1.f, 1.f, 0.f},
                               glm::vec3{1.f, 0.f, 1.f}, glm::vec3{0.f, 1.f, 1.f},
                               glm::vec3{1.f, 1.f, 1.f}};
    for (auto& light_color : light_colors) {
        auto point_light = std::make_shared<effects::PointLightEffect>(
            *resourceManager, frame_layout, 1.f, .04f, light_color);
        world.addDrawable(point_light);
    }

    auto delta_particle =
        std::make_shared<effects::DeltaParticle>(*resourceManager, frame_layout, PARTICLE_COUNT);
    auto light_model = std::make_shared<effects::ModelForMultiMesh>(*resourceManager, frame_layout,
                                                                    names, "model");
    world.addDrawable(light_model);
    auto sky_box = std::make_shared<effects::SkyBox>(*resourceManager, frame_layout);
    world.addDrawable(sky_box);
    PickingSystem::commit();
}

}  // namespace graphics
