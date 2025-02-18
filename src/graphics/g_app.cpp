#include "g_app.hpp"

#include "config/window.h"
#include "g_defines.hpp"
#include "resource/image.hpp"
#include <spdlog/spdlog.h>
#include "render_core/render_vulkan/render_vulkan.hpp"
#include "glfw_window.hpp"
#include "sdl_window.hpp"
#include <thread>
#include "model.hpp"
#include "render_core/framebufferConfig.hpp"

namespace g {
namespace {}  // namespace

void App::run() {
    auto* graphics = render_base->getGraphics();
    ::std::string s(image_path + "viking_room.png");
    ::std::string s2(image_path + "p1.jpg");
    resource::image::Image img(s);
    resource::image::Image img2(s2);
    graphics->addTexture(img.getImageInfo());

    // graphics->addTexture(img2.getImageInfo());
    auto model = graphics::Model::createFromFile("models/viking_room.obj");
    std::span<float> verticesSpan(reinterpret_cast<float*>(model->vertices_.data()),
                                  model->vertices_.size() * sizeof(Model::Vertex) / sizeof(float));

    while (!window->shouldClose()) {

        render::frame::FramebufferConfig frames{
            .width = 1920, .height = 1080, .stride = 1920};
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        window->pullEvents();

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time =
            std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime)
                .count();

        UniformBufferObject ubo{};
        ubo.model =
            glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f),
                                    (float)window->getFramebufferLayout().width /
                                        (float)window->getFramebufferLayout().height,
                                    0.1f, 10.0f);
        ubo.proj[1][1] *= -1;
        graphics->addUniformBuffer(&ubo, sizeof(ubo));
        graphics->addVertex(verticesSpan, model->indices_);
        graphics->drawIndics(model->indices_.size());
        auto& shader_notify = render_base->getShaderNotify();
        const int shaders_building = shader_notify.ShadersBuilding();

        if (shaders_building > 0) {
            window->setWindowTitle(fmt::format("Building {} shader(s)", shaders_building));
        } else {
            window->setWindowTitle("graphics");
        }
        render_base->composite(std::span{&frames, 1});
    }
}

App::App(const Config& config) {
    auto window_config = config.getConfig<config::window::Window>();
    // window = std::make_unique<ScreenWindow>(
    //     ScreenExtent{.width = window_config.width, .height = window_config.height},
    //     window_config.title);
    window = std::make_unique<graphics::SDLWindow>(window_config.width,window_config.height,
            window_config.title);

    render_base = std::make_unique<render::vulkan::RendererVulkan>(window.get());
}

App::~App(){};

}  // namespace g
