#include "app.hpp"

#include "config/vulkan.h"
#include "config/window.h"
#include "core/buffer.hpp"
#include "g_defines.hpp"
#include "g_descriptor.hpp"
#include "g_game_object.hpp"
#include "g_imgui.hpp"
#include "g_render.hpp"
#include "g_render_system.hpp"
// imgui begin
//#include "g_imgui.hpp"
#include "resource/image_texture.hpp"
// imgui end
#include <spdlog/spdlog.h>
#include "glfw_window.hpp"

#include <chrono>
#include <thread>

namespace graphics {
namespace {
auto create_descriptor_pool(int count) -> ::std::unique_ptr<g::DescriptorPool> {
    return g::DescriptorPool::Builder()
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
auto loadGameObjects() -> g::GameObject::Map {
    g::GameObject::Map gameObjects;
    auto cube = g::GameObject::createGameObject();
    cube.model = g::Model::createFromFile("models/viking_room.obj");
    gameObjects.emplace(cube.getId(), ::std::move(cube));
    return gameObjects;
}
}  // namespace

void App::run() {
    core::Device device_;
    auto gameObjects = loadGameObjects();
    ::std::unique_ptr<g::DescriptorPool> descriptorPool_ = create_descriptor_pool(1000);
    auto setLayout = g::DescriptorSetLayout::Builder()
                         .addBinding(0, ::vk::DescriptorType::eUniformBuffer,
                                     ::vk::ShaderStageFlagBits::eAllGraphics)
                         .addBinding(1, ::vk::DescriptorType::eCombinedImageSampler,
                                     ::vk::ShaderStageFlagBits::eAllGraphics)
                         .build();

    ::std::vector<::std::unique_ptr<core::Buffer>> uboBuffers(2);
    for (auto& uboBuffer : uboBuffers) {
        uboBuffer = ::std::make_unique<core::Buffer>(device_.createBuffer(
            sizeof(g::UniformBufferObject), 1, ::vk::BufferUsageFlagBits::eUniformBuffer,
            ::vk::MemoryPropertyFlagBits::eHostVisible |
                ::vk::MemoryPropertyFlagBits::eHostCoherent));
        uboBuffer->map();
    }
    ::std::string s(image_path + "viking_room.png");
    resource::image::Image img(s);
    resource::image::ImageTexture imageTexture{device_, img, g::Swapchain::DEFAULT_COLOR_FORMAT};

    ::std::vector<::vk::DescriptorSet> descriptorSets(uboBuffers.size());
    for (int i = 0; auto& descriptorSet : descriptorSets) {
        auto bufferInfo = uboBuffers[i++]->descriptorInfo();
        descriptorSet = g::DescriptorWriter()
                            .writeBuffer((*setLayout)(0), bufferInfo)
                            .writeImage((*setLayout)(1), imageTexture.descriptorImageInfo())
                            .build(*descriptorPool_, (*setLayout)());
    }

    g::RenderProcessor render([this]() { return dynamic_cast<g::ScreenWindow*>(window.get())->getExtent(); });
    g::RenderSystem renderSystem(device_, static_cast<::vk::RenderPass>(render), (*setLayout)());
    g::Imgui imgui((*dynamic_cast<g::ScreenWindow*>(window.get()))(), descriptorPool_->getDescriptorPool(),
                static_cast<::vk::RenderPass>(render),
                window->getWindowSystemInfo().render_surface_scale);
    while (!window->shouldClose()) {
        glfwPollEvents();
        if (window->IsMinimized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        g::UniformBufferObject ubo = imgui.get_uniform_buffer(render.extentAspectRation());

        if (render.beginFrame()) {
            uboBuffers[render.getCurrentFrameIndex()]->writeToBuffer(&ubo);
            uboBuffers[render.getCurrentFrameIndex()]->flush();

            g::FrameInfo frameInfo{.frameIndex = render.getCurrentFrameIndex(),
                                .commandBuffer = render.getCurrentCommandBuffer(),
                                .descriptorSet = descriptorSets[render.getCurrentFrameIndex()],
                                .gameObjects = gameObjects};

            render.beginSwapchainRenderPass();
            renderSystem.render(frameInfo);
            imgui.draw(render.getCurrentCommandBuffer());
            render.endSwapchainRenderPass();
            render.endFrame();
        }
    }

    core::Device::waitIdle();
}

App::App(const g::Config& config) {
    auto window_config = config.getConfig<config::window::Window>();
    window = std::make_unique<g::ScreenWindow>(
        g::ScreenExtent{.width = window_config.width, .height = window_config.height},
        window_config.title);

    auto vulkan_config = config.getConfig<config::vulkan::Vulkan>();
    auto requiredInstanceExtends =
        g::ScreenWindow::getRequiredInstanceExtends(vulkan_config.validation_layers);
    auto deviceExtensions = config::getDeviceExtensions();

    core::Device::init(
        requiredInstanceExtends, deviceExtensions,
        [this](VkInstance instance) -> VkSurfaceKHR {
            return dynamic_cast<g::ScreenWindow*>(window.get())->getSurface(instance);
            // return render::vulkan::createSurface(instance, window->getWindowSystemInfo());
        },
        vulkan_config.validation_layers);
}

App::~App() { core::Device::destroy(); };

}  // namespace g
