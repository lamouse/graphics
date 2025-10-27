#include "app.hpp"

#include "config/window.h"
#include "resource/model_instance.hpp"
#include "resource/particle_instance.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/components/camera_component.hpp"
#include "system/setting_ui.hpp"
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
    auto* shader_cache = graphics->getShaderCache();
    std::string model_shader_name = "model";
    std::string particle_shader_name = "particle";
    auto vertex_hash = shader_cache->addShader(
        resourceManager.getShaderCode(render::ShaderType::Vertex, model_shader_name),
        render::ShaderType::Vertex);
    auto fragment_hash = shader_cache->addShader(
        resourceManager.getShaderCode(render::ShaderType::Fragment, model_shader_name),
        render::ShaderType::Fragment);

    auto particle_vertex_hash = shader_cache->addShader(
        resourceManager.getShaderCode(render::ShaderType::Vertex, particle_shader_name),
        render::ShaderType::Vertex);
    auto particle_fragment_hash = shader_cache->addShader(
        resourceManager.getShaderCode(render::ShaderType::Fragment, particle_shader_name),
        render::ShaderType::Fragment);
    // auto particle_compute_hash = shader_cache->addShader(
    //     resourceManager.getShaderCode(render::ShaderType::Compute, particle_shader_name),
    //     render::ShaderType::Compute);

    world::World world;
    [[maybe_unused]] bool show_console_logger = false;
    std::string viking_room_path = image_path + "viking_room.png";
    std::string other_image = image_path + "p1.jpg";
    std::string viking_obj_path = "models/viking_room.obj";
    resourceManager.addTexture(viking_room_path,
                               [&](const resource::image::ITexture& texture) -> render::TextureId {
                                   return graphics->uploadTexture(texture);
                               });
    resourceManager.addTexture(other_image,
                               [&](const resource::image::ITexture& texture) -> render::TextureId {
                                   return graphics->uploadTexture(texture);
                               });

    resourceManager.addMesh(viking_obj_path, [&](const auto& mesh) -> render::MeshId {
        return graphics->uploadModel(mesh);
    });

    Particle particle;
    resourceManager.addMesh("particle", particle, [&](const auto& mesh) -> render::MeshId {
        return graphics->uploadModel(mesh);
    });
    ParticleInstance particle_instance;
    particle_instance.meshId = resourceManager.getMesh("particle");
    ModelInstance modelInstance = ModelInstance::createGameObject(
        resourceManager.getTexture(viking_room_path), resourceManager.getMesh(viking_obj_path));
    ModelInstance modelInstance2 = ModelInstance::createGameObject(
        resourceManager.getTexture(other_image), resourceManager.getMesh(viking_obj_path));
    auto& camera =
        world.getEntity(world::WorldEntityType::CAMERA).getComponent<ecs::CameraComponent>();
    auto& modelComponent = modelInstance.getEntity().getComponent<ecs::TransformComponent>();
    while (!window->shouldClose()) {
        shader_cache->setCurrentShader(vertex_hash, fragment_hash);

        window->pullEvents();
        if (window->IsMinimized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (camera.extentAspectRation != window->getAspectRatio()) {
            camera.extentAspectRation = window->getAspectRatio();
        }
        modelComponent.rotation.z = getRuntime() * glm::radians(90.0F);
        if ((static_cast<int>(modelComponent.rotation.z) % 10) > 5) {
            modelInstance.setTextureId(resourceManager.getTexture(other_image));
            modelInstance2.setTextureId(resourceManager.getTexture(viking_room_path));
        } else {
            modelInstance.setTextureId(resourceManager.getTexture(viking_room_path));
            modelInstance2.setTextureId(resourceManager.getTexture(other_image));
        }
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
        model2.scale.x = modelComponent.scale.x * 0.4F;
        model2.scale.y = modelComponent.scale.y * 0.4F;
        model2.scale.z = modelComponent.scale.z * 0.4F;
        ubo2.model = model2.mat4();
        modelInstance2.writeToUBOMapData(ubo2.as_byte_span());
        graphics->draw(modelInstance2);

        shader_cache->setCurrentShader(particle_vertex_hash, particle_fragment_hash);
        graphics->draw(particle_instance);
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
            draw_setting();
            ui::begin();
            ui::draw_result(imageId, window->getAspectRatio());
            ui::pipeline_state(pipeline_state);
            logger.drawUi(show_console_logger);
            world.drawUI();
            modelInstance.drawUI();
            ui::end();
        });
#endif
        render_base->composite(std::span{&frames, 1});
        graphics->clean();
    }
}

App::App(const g::Config& config) : logger() {
    auto [width, height, title] = config.getConfig<config::window::Window>();

#if defined(USE_GLFW)
    window = std::make_unique<ScreenWindow>(width, height, title);
#endif
#if defined(USE_SDL)
    window = std::make_unique<graphics::SDLWindow>(width, height, title);
#endif
    render_base = std::make_unique<render::vulkan::RendererVulkan>(window.get());
}

App::~App() {};

}  // namespace graphics
