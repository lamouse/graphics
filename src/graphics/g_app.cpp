#include "g_app.hpp"

#include "config/vulkan.h"
#include "config/window.h"
#include "core/buffer.hpp"
#include "g_defines.hpp"
#include "g_descriptor.hpp"
#include "g_game_object.hpp"
#include "g_render.hpp"
#include "g_render_system.hpp"
#include "resource/image.hpp"
#include "render_core/surface.hpp"
#include <spdlog/spdlog.h>
#include "render_core/render_vulkan/render_vulkan.hpp"
#include "glfw_window.hpp"
#include <thread>
#include "render_core/framebufferConfig.hpp"

namespace g {
namespace {
auto create_descriptor_pool(int count) -> ::std::unique_ptr<DescriptorPool> {
    return DescriptorPool::Builder()
        .setPoolFlags(::vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
        .setMaxSets(count)
        .addPoolSize(::vk::DescriptorType::eUniformBuffer, count)
        .addPoolSize(::vk::DescriptorType::eSampler, count)
        .addPoolSize(::vk::DescriptorType::eCombinedImageSampler, count)
        .addPoolSize(::vk::DescriptorType::eStorageImage, count)
        .addPoolSize(::vk::DescriptorType::eSampledImage, count)
        .addPoolSize(::vk::DescriptorType::eUniformTexelBuffer, count)
        .addPoolSize(::vk::DescriptorType::eStorageTexelBuffer, count)
        .addPoolSize(::vk::DescriptorType::eStorageBuffer, count)
        .addPoolSize(::vk::DescriptorType::eStorageBufferDynamic, count)
        .addPoolSize(::vk::DescriptorType::eUniformBufferDynamic, count)
        .addPoolSize(::vk::DescriptorType::eInputAttachment, count)
        .build();
}
auto loadGameObjects() -> GameObject::Map {
    GameObject::Map gameObjects;
    auto cube = GameObject::createGameObject();
    cube.model = Model::createFromFile("models/viking_room.obj");
    gameObjects.emplace(cube.getId(), ::std::move(cube));
    return gameObjects;
}
}  // namespace

void App::run() {
    auto* graphics = render_base->getGraphics();
    ::std::string s(image_path + "viking_room.png");
    ::std::string s2(image_path + "p1.jpg");
    resource::image::Image img(s);
    resource::image::Image img2(s2);

    // graphics->addTexture(img2.getImageInfo());
    auto model = Model::createFromFile("models/viking_room.obj");
    std::span<float> verticesSpan(reinterpret_cast<float*>(model->vertices_.data()),
                                  model->vertices_.size() * sizeof(Model::Vertex) / sizeof(float));

    while (!window->shouldClose()) {
        auto layout = window->getFramebufferLayout();
        render::frame::FramebufferConfig frames{
            .width = layout.width, .height = layout.height, .stride = 1};
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        render_base->composite(std::span{&frames, 1});

        window->pullEvents();

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time =
            std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime)
                .count();

        UniformBufferObject ubo{};
        ubo.model =
            glm::rotate(glm::mat4(1.0f), 1 * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f),
                                    (float)window->getFramebufferLayout().width /
                                        (float)window->getFramebufferLayout().height,
                                    0.1f, 10.0f);
        ubo.proj[1][1] *= -1;
        graphics->addUniformBuffer(&ubo, sizeof(ubo));
        graphics->addTexture(img.getImageInfo());

        graphics->addVertex(verticesSpan, model->indices_);
        graphics->drawIndics(model->indices_.size());
        auto& shader_notify = render_base->getShaderNotify();
        const int shaders_building = shader_notify.ShadersBuilding();

        if (shaders_building > 0) {
            window->setWindowTitle(fmt::format("Building {} shader(s)", shaders_building));
        } else {
            window->setWindowTitle("graphics");
        }
    }
}

App::App(const Config& config) {
    auto window_config = config.getConfig<config::window::Window>();
    window = std::make_unique<ScreenWindow>(
        ScreenExtent{.width = window_config.width, .height = window_config.height},
        window_config.title);

    render_base = std::make_unique<render::vulkan::RendererVulkan>(window.get());
}

App::~App() { core::Device::destroy(); };

}  // namespace g
