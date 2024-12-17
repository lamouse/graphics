#include "g_app.hpp"

#include "core/buffer.hpp"
#include "g_defines.hpp"
#include "g_render.hpp"
#include "g_render_system.hpp"
#include "g_descriptor.hpp"
#include "g_game_object.hpp"

// imgui begin
#include "g_imgui.hpp"
#include "resource/image_texture.hpp"
// imgui end
#include <spdlog/spdlog.h>

#include <chrono>
#include <thread>

namespace g {
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
    resource::image::ImageTexture imageTexture{device_, img, DEFAULT_FORMAT};

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
    Imgui imgui;
    imgui.init(window(), descriptorPool_->getDescriptorPool(), window.getScale());
    static auto startTime = ::std::chrono::high_resolution_clock::now();
    Camera camera{};
    ImguiDebugInfo debugInfo{};
    debugInfo.speed = 90.0F;
    debugInfo.look_x = 2.0f;
    debugInfo.look_y = 2.0f;
    debugInfo.look_z = 2.0F;
    debugInfo.up_z = 1.f;
    debugInfo.rotate_z = 2.0;
    debugInfo.radians = 45.f;
    debugInfo.z_far = .1f;
    debugInfo.z_near = 10.f;
    debugInfo.center_x = 0;
    debugInfo.center_y = 0;
    debugInfo.center_z = 0;
    while (!window.shouldClose()) {
        glfwPollEvents();

        if (glfwGetWindowAttrib(window(), GLFW_ICONIFIED) != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        auto currentTime = ::std::chrono::high_resolution_clock::now();
        float time = ::std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        UniformBufferObject ubo{};
        ubo.model = ::glm::rotate(glm::mat4(1.0f), time * glm::radians(debugInfo.speed),
                                  glm::vec3(debugInfo.rotate_x, debugInfo.rotate_y, debugInfo.rotate_z));
        ubo.view = ::glm::lookAt(glm::vec3(debugInfo.look_x, debugInfo.look_y, debugInfo.look_z),
                                 glm::vec3(debugInfo.center_x, debugInfo.center_y, debugInfo.center_z),
                                 glm::vec3(debugInfo.up_x, debugInfo.center_y, debugInfo.up_z));
        ubo.proj = ::glm::perspective(glm::radians(debugInfo.radians), render.extentAspectRation(), debugInfo.z_far,
                                      debugInfo.z_near);
        ubo.proj[1][1] *= -1;

        if (render.beginFrame()) {
            uboBuffers[render.getCurrentFrameIndex()]->writeToBuffer(&ubo);
            uboBuffers[render.getCurrentFrameIndex()]->flush();

            FrameInfo frameInfo{render.getCurrentFrameIndex(),
                                time,
                                render.getCurrentCommandBuffer(),
                                camera,
                                descriptorSets[render.getCurrentFrameIndex()],
                                gameObjects};

            render.beginSwapchainRenderPass();
            renderSystem.render(frameInfo);
            render.endSwapchainRenderPass();
            render.endFrame();
        }
        g::Imgui::draw(debugInfo);
    }

    device_.logicalDevice().waitIdle();
}

App::App() {
    core::Device::init(
        Window::getRequiredInstanceExtends(enableValidationLayers), deviceExtensions,
        [this](VkInstance instance) -> VkSurfaceKHR { return window.getSurface(instance); }, enableValidationLayers);
}

App::~App() { core::Device::destroy(); };

}  // namespace g
