#pragma once

#include "g_pipeline.hpp"
#include "g_swapchain.hpp"
#include <memory>
namespace g{
class RenderProcesser
{
private:
    ::std::unique_ptr<Swapchain> swapchain;
    bool isFrameStart{false};
    uint32_t currentImageIndex;
    int currentFrameIndex{0};
    ::std::vector<::vk::CommandBuffer> commandBuffers_;

    void createSwapchain();
    void allcoCmdBuffer();
public:
    bool beginFrame();
    void beginSwapchainRenderPass();
    void endSwapchainRenderPass();
    void endFrame();
    void render(::std::vector<GameObject> gameObjects);
    ::vk::RenderPass getRenderPass(){return swapchain->getRenderPass();}
    ::vk::CommandBuffer& getCurrentCommadBuffer(){return commandBuffers_[currentFrameIndex];};
    RenderProcesser();
    ~RenderProcesser();
};

}