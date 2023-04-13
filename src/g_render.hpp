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
    ::vk::RenderPass renderPass_;
    ::vk::SampleCountFlagBits sampleCount_;
    void createSwapchain();
    void createRenderPass();
    void allcoCmdBuffer();
public:
    bool beginFrame();
    void beginSwapchainRenderPass(::vk::Framebuffer* buffer=nullptr, ::vk::RenderPass* renderPass = nullptr);
    void endSwapchainRenderPass();
    void endFrame();
    ::vk::RenderPass getRenderPass(){return renderPass_;}
    ::vk::CommandBuffer& getCurrentCommadBuffer(){return commandBuffers_[currentFrameIndex];};
    int getCurrentFrameIndex(){return currentFrameIndex;}
    RenderProcesser(core::Device& device);
    ~RenderProcesser();
    float extentAspectRation(){return swapchain->extentAspectRation();}
};

}