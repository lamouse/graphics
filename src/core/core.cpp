// module;

#include <atomic>
#include "core/core.hpp"

#include "render_core/render_core.hpp"
#include "resource/resource.hpp"
#include "world/world.hpp"
#include "effects/model/multi_mesh_model.hpp"
#include "graphics/gui.hpp"
#include "resource/mesh_instance.hpp"
#include "system/pick_system.hpp"
#include "system/setting_ui.hpp"
#include "input/input.hpp"
#include "input/mouse.h"
#include "input/drop.hpp"
#include "input/keyboard.hpp"
#include "common/logger.hpp"
#include "system/logger_system.hpp"
#include <spdlog/spdlog.h>
#include "common/file.hpp"
#include <filesystem>
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

            std::vector<std::string> shader_names{model_shader_name, particle_shader,
                                                  point_light_shader_name};
            for (auto& shader_name : shader_names) {
                resourceManager->addGraphShader(shader_name);
            }
            resourceManager->addComputeShader(particle_shader);

            graphics::PickingSystem::commit();
        }

    public:
        auto Render() -> render::RenderBase* { return render_base.get(); }
        auto isSisShutdown() -> bool { return is_shut_down_.load(); }
        void set_shutdown(bool shutdown) { is_shut_down_ = shutdown; }
        void shutdown_main_process() {
            if (render_base) {
                Render()->composite(std::span{&frame_config_, 1});
            }
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

            frame_config_ = {.width = settings::values.resolution.width,
                             .height = settings::values.resolution.height,
                             .stride = settings::values.resolution.width};  // NOLINT
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
            graphics::ui::run_all_imgui_event();
            auto input_system_ = std::move(input_system);
            auto* file_drop = input_system_->GetFileDrop();
            while (!file_drop->empty()) {
                auto drop_file = file_drop->pop();
                spdlog::debug("drop file: {}, type: {}", drop_file,
                              common::to_string(common::getFileType(drop_file)));

                if (common::getFileType(drop_file) == common::FileType::Model) {
                    try {
                        std::filesystem::path model_path(drop_file);
                        common::copy_file(drop_file, common::get_current_path() + "/models/" +
                                                         model_path.filename().string());

                        auto mtl_file_path = common::model_mtl_file_path(drop_file);
                        if (!mtl_file_path.empty()) {
                            auto dst_mtl_path = model_path;
                            dst_mtl_path.replace_extension(".mtl");
                            common::copy_file(mtl_file_path, common::get_current_path() +
                                                                 "/models/" +
                                                                 dst_mtl_path.filename().string());
                        }
                        auto* resourceManager = resource_manager.get();
                        graphics::ModelResourceName names{
                            .shader_name = "model", .mesh_name = model_path.filename().string()};
                        auto light_model = std::make_shared<graphics::effects::ModelForMultiMesh>(
                            *resourceManager, names, "model");
                        world_->addDrawable(light_model);
                    } catch (std::filesystem::filesystem_error& e) {
                        spdlog::error("file system error {}", e.what());
                    }
                }
            }

            auto* window = Render()->GetRenderWindow();
            auto* graphics = Render()->getGraphics();
            graphics->clean(frameClean);
            input_system_->GetMouse()->setCapture(graphics::ui::IsMouseControlledByImGui());
            input_system_->GetKeyboard()->setCapture(graphics::ui::IsKeyboardControlledByImGui());
            world_->update(*window, *resource_manager, *input_system_);

            world_->draw(graphics);

            if (settings::values.use_debug_ui.GetValue()) {
                auto& shader_notify = Render()->getShaderNotify();
                const int shaders_building = shader_notify.ShadersBuilding();
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
                statusData.registry_count = static_cast<int>(world_->get_module_count());
                auto ui_fun = [&]() -> void {
                    auto imageId = graphics->getDrawImage();
                    graphics::ui::show_menu(settings::values.menu_data);
                    graphics::draw_setting(settings::values.menu_data.show_system_setting);
                    graphics::ui::showOutliner(*world_, *resource_manager,
                                               settings::values.menu_data);
                    render_status_bar(settings::values.menu_data, statusData);
                    graphics::ui::draw_texture(settings::values.menu_data, imageId,
                                               window->getAspectRatio());
                    common::logger::getLogger()->drawUi(settings::values.menu_data.show_log);
                };
                Render()->composite(std::span{&frame_config_, 1}, ui_fun);
            } else {
                Render()->composite(std::span{&frame_config_, 1});
            }
        }
};
System::System() : impl_(std::make_unique<System::Impl>()) {}
System::~System() = default;
void System::load(core::frontend::BaseWindow& window) { impl_->load(window); }
auto System::Render() -> render::RenderBase& { return *impl_->Render(); }
[[nodiscard]] auto System::Render() const -> render::RenderBase& { return *impl_->Render(); }
void System::setShutdown(bool shutdown) { impl_->set_shutdown(shutdown); }
auto System::isShutdown() -> bool { return impl_->isSisShutdown(); }
void System::shutdownMainProcess() { impl_->shutdown_main_process(); }
auto System::ResourceManager() -> graphics::ResourceManager& { return *impl_->ResourceManager(); }
auto System::ResourceManager() const -> graphics::ResourceManager& {
    return *impl_->ResourceManager();
}

void System::run(std::shared_ptr<graphics::input::InputSystem> inputSystem) {
    impl_->run(std::move(inputSystem));
}
}  // namespace core