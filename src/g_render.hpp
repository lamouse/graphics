#pragma once

#include "g_pipeline.hpp"
#include "g_swapchain.hpp"
#include "g_command.hpp"
#include <memory>
namespace g{
class RenderProcesser
{
private:
    ::std::unique_ptr<Command> command;
    ::std::unique_ptr<Swapchain> swapchain;
    bool isFrameStart{false};
    uint32_t currentImageIndex;
    int currentFrameIndex{0};
    void createSwapchain();
public:
    bool beginFrame();
    void beginSwapchainRenderPass();
    void endSwapchainRenderPass();
    void endFrame();
    void render(::std::vector<GameObject> gameObjects);
    ::vk::RenderPass getRenderPass(){return swapchain->getRenderPass();}
    ::vk::CommandBuffer getCurrentCommadBuffer(){return command->getCommandBuffer(currentFrameIndex);};
    RenderProcesser();
    ~RenderProcesser();
};

}