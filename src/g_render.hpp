#pragma once

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
    auto beginFrame() -> bool;
    void beginSwapchainRenderPass();
    void endSwapchainRenderPass();
    void endFrame();
    auto getRenderPass(){return renderPass_;}
    auto getCurrentCommadBuffer() -> ::vk::CommandBuffer&{return commandBuffers_[currentFrameIndex];};
    [[nodiscard]] auto getCurrentFrameIndex() const ->int{return currentFrameIndex;}
    RenderProcesser(core::Device& device);
    ~RenderProcesser();
    auto extentAspectRation()->auto{return swapchain->extentAspectRation();}
};

}