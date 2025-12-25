// module;

#include <atomic>
#include "core/core.hpp"

#include "render_core/render_core.hpp"
#include "resource/resource.hpp"
#include "world/world.hpp"
#include "effects/particle/particle.hpp"
#include "effects/light/point_light.hpp"
#include "effects/cubemap/skybox.hpp"
#include "effects/model/multi_mesh_model.hpp"
#include "graphics/gui.hpp"
#include "resource/mesh_instance.hpp"
#include "system/pick_system.hpp"
#include "system/setting_ui.hpp"
#include "input/input.hpp"
#include "input/mouse.h"
#include "system/logger_system.hpp"
import common;
// module core;

namespace core {
struct System::Impl {
    private:
        std::atomic<bool> is_shut_down_;
        std::unique_ptr<render::RenderBase> render_base;
        std::unique_ptr<graphics::ResourceManager> resource_manager;
        std::unique_ptr<world::World> world_;

        render::frame::FramebufferConfig frame_config_;
        render::CleanValue frameClean{};
        graphics::ui::StatusBarData statusData;

        void load_resource() {
            std::string viking_obj_path = "backpack";
            std::string model_shader_name = "model";
            std::string particle_shader = "particle";
            std::string point_light_shader_name = "point_light";
            auto* resourceManager = resource_manager.get();
            resourceManager->addGraphShader(model_shader_name);
            resourceManager->addGraphShader(particle_shader);
            resourceManager->addGraphShader(point_light_shader_name);
            resourceManager->addComputeShader(particle_shader);
            auto* window = render_base->GetRenderWindow();
            auto frame_layout = window->getFramebufferLayout();

            graphics::ModelResourceName names{.shader_name = model_shader_name,
                                              .mesh_name = viking_obj_path};

            std::array light_colors = {glm::vec3{1.f, 0.f, 0.f}, glm::vec3{0.f, 1.f, 0.f},
                                       glm::vec3{0.f, 0.f, 1.f}, glm::vec3{1.f, 1.f, 0.f},
                                       glm::vec3{1.f, 0.f, 1.f}, glm::vec3{0.f, 1.f, 1.f},
                                       glm::vec3{1.f, 1.f, 1.f}};
            for (auto& light_color : light_colors) {
                auto point_light = std::make_shared<graphics::effects::PointLightEffect>(
                    *resourceManager, frame_layout, 1.f, .04f, light_color);
                world_->addDrawable(point_light);
            }

            auto delta_particle = std::make_shared<graphics::effects::DeltaParticle>(
                *resourceManager, frame_layout, PARTICLE_COUNT);
            auto light_model = std::make_shared<graphics::effects::ModelForMultiMesh>(
                *resourceManager, frame_layout, names, "model");
            world_->addDrawable(light_model);
            auto sky_box =
                std::make_shared<graphics::effects::SkyBox>(*resourceManager, frame_layout);
            world_->addDrawable(sky_box);
            graphics::PickingSystem::commit();
        }

    public:
        auto Render() -> render::RenderBase* { return render_base.get(); }
        auto isSisShutdown() -> bool { return is_shut_down_.load(); }
        void set_shutdown(bool shutdown) { is_shut_down_ = shutdown; }
        void shutdown_main_process() {
            world_.reset();
            resource_manager.reset();
            render_base.reset();
        }
        auto ResourceManager() -> graphics::ResourceManager* { return resource_manager.get(); }
        void load(frontend::BaseWindow& window) {
            shutdown_main_process();
            render_base = render::createRender(&window);
            resource_manager =
                std::make_unique<graphics::ResourceManager>(render_base->getGraphics());
            world_ = std::make_unique<world::World>();

            frame_config_ = {.width = settings::values.resolution.weight,
                             .height = settings::values.resolution.height,
                             .stride = settings::values.resolution.weight};  // NOLINT
            frameClean.width = frame_config_.width;
            frameClean.hight = frame_config_.height;
            frameClean.framebuffer.color_formats.at(0) =
                render::surface::PixelFormat::B8G8R8A8_UNORM;
            frameClean.framebuffer.depth_format = render::surface::PixelFormat::D32_FLOAT;
            frameClean.framebuffer.extent = {
                .width = frame_config_.width, .height = frame_config_.height, .depth = 1};
            statusData.device_name = render_base->GetDeviceVendor();
            load_resource();
        }

        void run(std::shared_ptr<graphics::input::InputSystem> input_system) {
            auto input_system_ = std::move(input_system);
            auto* window = Render()->GetRenderWindow();
            auto* graphics = Render()->getGraphics();
            graphics->clean(frameClean);
            world_->update(*window, *resource_manager, *input_system_);

            world_->draw(graphics);

            auto& shader_notify = Render()->getShaderNotify();
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
                    graphics::ui::show_menu(settings::values.menu_data);
                    graphics::draw_setting(settings::values.menu_data.show_system_setting);
                    graphics::ui::showOutliner(*world_, settings::values.menu_data);
                    render_status_bar(settings::values.menu_data, statusData);
                    graphics::ui::draw_texture(settings::values.menu_data, imageId,
                                               window->getAspectRatio());
                    common::logger::getLogger()->drawUi(settings::values.menu_data.show_log);
                };
                Render()->composite(std::span{&frame_config_, 1}, ui_fun);
            } else {
                Render()->composite(std::span{&frame_config_, 1});
            }

            world_->cleanLight();
        }
};
System::System() : impl_(std::make_unique<System::Impl>()) {}
System::~System() = default;
void System::load(core::frontend::BaseWindow& window) { impl_->load(window); }
auto System::Render() -> render::RenderBase& { return *impl_->Render(); }
[[nodiscard]] auto System::Render() const -> render::RenderBase& { return *impl_->Render(); }
void System::setShutdown(bool shutdown) { impl_->set_shutdown(shutdown); }
auto System::isShutdown() -> bool { return impl_->isSisShutdown(); }
void System::shutdownMainProcess() {}
auto System::ResourceManager() -> graphics::ResourceManager& { return *impl_->ResourceManager(); }
auto System::ResourceManager() const -> graphics::ResourceManager& {
    return *impl_->ResourceManager();
}

void System::run(std::shared_ptr<graphics::input::InputSystem> inputSystem) {
    impl_->run(std::move(inputSystem));
}
}  // namespace core