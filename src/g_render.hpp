#pragma once

#include <memory>
#include <functional>
#include "g_swapchain.hpp"
#include "g_defines.hpp"

namespace g {
class RenderProcessor {
    public:
        using getScreenExtendFunc = ::std::function<::g::ScreenExtent()>;

    private:
        ::std::unique_ptr<Swapchain> swapchain;
        bool isFrameStart{false};
        uint32_t currentImageIndex;
        uint32_t currentFrameIndex{0};
        ::std::vector<::vk::CommandBuffer> commandBuffers_;
        ::vk::RenderPass renderPass_;
        void createSwapchain();
        void createRenderPass();
        void allcoCmdBuffer();
        getScreenExtendFunc getScreenExtend_;
    public:

        auto beginFrame() -> bool;
        void beginSwapchainRenderPass();
        void endSwapchainRenderPass();
        void endFrame();
        explicit operator auto() & { return renderPass_; }
        auto getCurrentCommandBuffer() -> ::vk::CommandBuffer& { return commandBuffers_[currentFrameIndex]; };
        [[nodiscard]] auto getCurrentFrameIndex() const -> uint32_t { return currentFrameIndex; }
        RenderProcessor(getScreenExtendFunc getScreenExtend);
        ~RenderProcessor();
        auto extentAspectRation() { return swapchain->extentAspectRation(); }
};

}  // namespace g