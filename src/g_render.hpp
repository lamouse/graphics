#pragma once

#include "g_pipeline.hpp"
#include "g_swapchain.hpp"
#include "core/device.hpp"
#include <memory>
namespace g{
class RenderProcesser
{
private:
    ::std::unique_ptr<Swapchain> swapchain;
    bool isFrameStart{false};
    uint32_t currentImageIndex;
    int currentFrameIndex{0};
    core::Device& device_;
    ::std::vector<::vk::CommandBuffer> commandBuffers_;

    void createSwapchain();
    void allcoCmdBuffer();
public:
    bool beginFrame();
    void beginSwapchainRenderPass(::vk::Framebuffer* buffer=nullptr, ::vk::RenderPass* renderPass = nullptr);
    void endSwapchainRenderPass();
    void endFrame();
    ::vk::RenderPass getRenderPass(){return swapchain->getRenderPass();}
    ::vk::CommandBuffer& getCurrentCommadBuffer(){return commandBuffers_[currentFrameIndex];};
    int getCurrentFrameIndex(){return currentFrameIndex;}
    ::std::vector<::vk::Framebuffer> createFrameBuffer(int count, ::vk::RenderPass& renderPass){return swapchain->createFrameBuffer(count, renderPass);}
    RenderProcesser(core::Device& device);
    ~RenderProcesser();
    float extentAspectRation(){return swapchain->extentAspectRation();}
};

}