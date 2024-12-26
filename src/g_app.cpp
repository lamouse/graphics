#include "g_app.hpp"

#include "config/vulkan.h"
#include "core/buffer.hpp"
#include "g_defines.hpp"
#include "g_descriptor.hpp"
#include "g_game_object.hpp"
#include "g_render.hpp"
#include "g_render_system.hpp"
// imgui begin
#include "g_imgui.hpp"
#include "resource/image_texture.hpp"
// imgui end
#include <spdlog/spdlog.h>

#include <chrono>
#include <thread>

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
    core::Device device_;
    auto gameObjects = loadGameObjects();
    ::std::unique_ptr<DescriptorPool> descriptorPool_ = create_descriptor_pool(1000);
    auto setLayout =
        DescriptorSetLayout::Builder()
            .addBinding(0, ::vk::DescriptorType::eUniformBuffer, ::vk::ShaderStageFlagBits::eAllGraphics)
            .addBinding(1, ::vk::DescriptorType::eCombinedImageSampler, ::vk::ShaderStageFlagBits::eAllGraphics)
            .build();

    ::std::vector<::std::unique_ptr<core::Buffer>> uboBuffers(2);
    for (auto& uboBuffer : uboBuffers) {
        uboBuffer = ::std::make_unique<core::Buffer>(device_, sizeof(UniformBufferObject), 1,
                                                     ::vk::BufferUsageFlagBits::eUniformBuffer,
                                                     ::vk::MemoryPropertyFlagBits::eHostVisible);
        uboBuffer->map();
    }
    ::std::string s(image_path + "viking_room.png");
    resource::image::Image img(s);
    resource::image::ImageTexture imageTexture{device_, img, Swapchain::DEFAULT_COLOR_FORMAT};

    ::std::vector<::vk::DescriptorSet> descriptorSets(uboBuffers.size());
    for (int i = 0; auto& descriptorSet : descriptorSets) {
        auto bufferInfo = uboBuffers[i++]->descriptorInfo();
        descriptorSet = DescriptorWriter()
                            .writeBuffer((*setLayout)(0), bufferInfo)
                            .writeImage((*setLayout)(1), imageTexture.descriptorImageInfo())
                            .build(*descriptorPool_, (*setLayout)());
    }

    RenderProcessor render([this]() { return window.getExtent(); });
    RenderSystem renderSystem(device_, static_cast<::vk::RenderPass>(render), (*setLayout)());
    Imgui imgui(window(), descriptorPool_->getDescriptorPool(), static_cast<::vk::RenderPass>(render),
                window.getScale());

    while (!window.shouldClose()) {
        glfwPollEvents();

        if (glfwGetWindowAttrib(window(), GLFW_ICONIFIED) != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        UniformBufferObject ubo = imgui.get_uniform_buffer(render.extentAspectRation());

        if (render.beginFrame()) {
            uboBuffers[render.getCurrentFrameIndex()]->writeToBuffer(&ubo);
            uboBuffers[render.getCurrentFrameIndex()]->flush();

            FrameInfo frameInfo{render.getCurrentFrameIndex(), render.getCurrentCommandBuffer(),
                                descriptorSets[render.getCurrentFrameIndex()], gameObjects};

            render.beginSwapchainRenderPass();
            renderSystem.render(frameInfo);
            imgui.draw(render.getCurrentCommandBuffer());
            render.endSwapchainRenderPass();
            render.endFrame();
        }
    }

    device_.logicalDevice().waitIdle();
}

App::App(const Config& config) {
    auto vulkan_config = config.getConfig<config::vulkan::Vulkan>();
    core::Device::init(
        Window::getRequiredInstanceExtends(vulkan_config.validation_layers), deviceExtensions,
        [this](VkInstance instance) -> VkSurfaceKHR { return window.getSurface(instance); },
        vulkan_config.validation_layers);
}

App::~App() { core::Device::destroy(); };

}  // namespace g
