#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include "g_game_object.hpp"
namespace g
{

class Command final
{
private:
    ::vk::CommandPool cmdPool_;
    ::std::vector<::vk::CommandBuffer> commandBuffers_;
    void initCmdPool();
    void allcoCmdBuffer(int count);
    static ::std::unique_ptr<Command> instance;
    std::vector<::vk::Fence*> imagesInFlight;
    void recordCommandBuffer(){};
    Command(int count);
    void renderGameObjects(::std::vector<GameObject>& gameObjects, ::vk::CommandBuffer& commandBuffer, ::vk::PipelineLayout layout);
public:
    static void init(int count);
    static void quit();
    
    ~Command();

    void begin(int index);
    void beginRenderPass(int index, ::vk::Pipeline& pipeline, ::vk::RenderPass& renderPass, ::vk::Extent2D& extent, vk::Framebuffer& frameBuffer);
    void run(int index, ::vk::Fence& fence, ::vk::PipelineLayout& layout, ::std::vector<GameObject>& gameObjects);
    void endRenderPass(int index);
    void end(int index, ::vk::SwapchainKHR& swapchain, ::vk::Semaphore& waitSemaphore, ::vk::Semaphore& signalSemaphore, ::vk::Fence& fence);

    static Command& getInstance(){return *instance;}
};

}