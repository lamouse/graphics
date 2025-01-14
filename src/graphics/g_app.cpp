#include "g_app.hpp"

#include "config/vulkan.h"
#include "config/window.h"
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
#include "render_core/render_vulkan/render_vulkan.hpp"
#include <chrono>
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
    auto layout = window->getFramebufferLayout();
    render::frame::FramebufferConfig frames{
        .width = layout.width, .height = layout.height, .stride = 1};
    render_base->composite(std::span{&frames, 1});

    // core::Device device_;
    // auto gameObjects = loadGameObjects();
    // ::std::unique_ptr<DescriptorPool> descriptorPool_ = create_descriptor_pool(1000);
    // auto setLayout = DescriptorSetLayout::Builder()
    //                      .addBinding(0, ::vk::DescriptorType::eUniformBuffer,
    //                                  ::vk::ShaderStageFlagBits::eAllGraphics)
    //                      .addBinding(1, ::vk::DescriptorType::eCombinedImageSampler,
    //                                  ::vk::ShaderStageFlagBits::eAllGraphics)
    //                      .build();

    // ::std::vector<::std::unique_ptr<core::Buffer>> uboBuffers(2);
    // for (auto& uboBuffer : uboBuffers) {
    //     uboBuffer = ::std::make_unique<core::Buffer>(device_.createBuffer(
    //         sizeof(UniformBufferObject), 1, ::vk::BufferUsageFlagBits::eUniformBuffer,
    //         ::vk::MemoryPropertyFlagBits::eHostVisible |
    //             ::vk::MemoryPropertyFlagBits::eHostCoherent));
    //     uboBuffer->map();
    // }
    // ::std::string s(image_path + "viking_room.png");
    // resource::image::Image img(s);
    // resource::image::ImageTexture imageTexture{device_, img, Swapchain::DEFAULT_COLOR_FORMAT};

    // ::std::vector<::vk::DescriptorSet> descriptorSets(uboBuffers.size());
    // for (int i = 0; auto& descriptorSet : descriptorSets) {
    //     auto bufferInfo = uboBuffers[i++]->descriptorInfo();
    //     descriptorSet = DescriptorWriter()
    //                         .writeBuffer((*setLayout)(0), bufferInfo)
    //                         .writeImage((*setLayout)(1), imageTexture.descriptorImageInfo())
    //                         .build(*descriptorPool_, (*setLayout)());
    // }

    // RenderProcessor render([this]() { return dynamic_cast<Window*>(window.get())->getExtent();
    // }); RenderSystem renderSystem(device_, static_cast<::vk::RenderPass>(render),
    // (*setLayout)()); Imgui imgui((*dynamic_cast<Window*>(window.get()))(),
    // descriptorPool_->getDescriptorPool(),
    //             static_cast<::vk::RenderPass>(render),
    //             window->getWindowSystemInfo().render_surface_scale);
    while (!window->shouldClose()) {
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // if (window->IsMinimized()) {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
        //     continue;
        // }
        // UniformBufferObject ubo = imgui.get_uniform_buffer(render.extentAspectRation());

        // if (render.beginFrame()) {
        //     uboBuffers[render.getCurrentFrameIndex()]->writeToBuffer(&ubo);
        //     uboBuffers[render.getCurrentFrameIndex()]->flush();

        //     FrameInfo frameInfo{.frameIndex = render.getCurrentFrameIndex(),
        //                         .commandBuffer = render.getCurrentCommandBuffer(),
        //                         .descriptorSet = descriptorSets[render.getCurrentFrameIndex()],
        //                         .gameObjects = gameObjects};

        //     render.beginSwapchainRenderPass();
        //     renderSystem.render(frameInfo);
        //     imgui.draw(render.getCurrentCommandBuffer());
        //     render.endSwapchainRenderPass();
        //     render.endFrame();
        // }
    }

    core::Device::waitIdle();
}

App::App(const Config& config) {
    auto window_config = config.getConfig<config::window::Window>();
    window = std::make_unique<Window>(
        ScreenExtent{.width = window_config.width, .height = window_config.height},
        window_config.title);

    auto vulkan_config = config.getConfig<config::vulkan::Vulkan>();
    auto requiredInstanceExtends =
        Window::getRequiredInstanceExtends(vulkan_config.validation_layers);
    auto deviceExtensions = config::getDeviceExtensions();
    render_base = std::make_unique<render::vulkan::RendererVulkan>(window.get());
    // core::Device::init(
    //     requiredInstanceExtends, deviceExtensions,
    //     [this](VkInstance instance) -> VkSurfaceKHR {
    //         return dynamic_cast<Window*>(window.get())->getSurface(instance);
    //         // return render::vulkan::createSurface(instance, window->getWindowSystemInfo());
    //     },
    //     vulkan_config.validation_layers);
}

App::~App() { core::Device::destroy(); };

}  // namespace g
