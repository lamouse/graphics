#pragma once

#include <memory>

#include "core/device.hpp"
#include "g_swapchain.hpp"

namespace g {
class RenderProcesser {
    private:
        ::std::unique_ptr<Swapchain> swapchain;
        bool isFrameStart{false};
        uint32_t currentImageIndex;
        uint32_t currentFrameIndex{0};
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
        operator auto() & { return renderPass_; }
        auto getCurrentCommadBuffer() -> ::vk::CommandBuffer& { return commandBuffers_[currentFrameIndex]; };
        [[nodiscard]] auto getCurrentFrameIndex() const -> uint32_t { return currentFrameIndex; }
        RenderProcesser(core::Device& device);
        ~RenderProcesser();
        auto extentAspectRation() { return swapchain->extentAspectRation(); }
};

}  // namespace g