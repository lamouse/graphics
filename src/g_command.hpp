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
    std::vector<::vk::Fence*> imagesInFlight;
    ::std::vector<::vk::Semaphore> renderFinshSemaphores;

    void initCmdPool();
    void allcoCmdBuffer(int count);
    void recordCommandBuffer(){};
    void createSemaphore(int count);
public:
    Command(int count);
    ~Command();
    void begin(int index);
    void beginRenderPass(int index, ::vk::RenderPass& renderPass, ::vk::Extent2D& extent, vk::Framebuffer& frameBuffer);
    void endRenderPass(int index);
    ::vk::Result end(uint32_t index, ::vk::SwapchainKHR& swapchain, ::vk::Semaphore& waitSemaphore, ::vk::Fence& fence);
    ::vk::CommandBuffer getCommandBuffer(int index){return commandBuffers_[index];}
};

}