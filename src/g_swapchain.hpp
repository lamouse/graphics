#ifndef G_SWAPCHAIN_HPP
#define G_SWAPCHAIN_HPP
#include <memory>
#include <vulkan/vulkan.hpp>

#include "core/device.hpp"

namespace g {

class Swapchain {
    public:
        struct SwapchainInfo {
                ::vk::Extent2D extent2D;
                uint32_t imageCount;
                ::vk::SurfaceFormatKHR formatKHR;
                ::vk::SurfaceTransformFlagBitsKHR transForm;
                ::vk::PresentModeKHR presentMode;
        };
        [[nodiscard]] auto getSwapchain() const -> ::vk::SwapchainKHR { return swapchain; }
        [[nodiscard]] auto getSwapchainColorFormat() const -> ::vk::Format;
        [[nodiscard]] auto getSwapchainDepthFormat() const -> ::vk::Format { return depthFormat; }

        auto acquireNextImage() -> ::vk::ResultValue<uint32_t>;
        auto submitCommand(::vk::CommandBuffer& commandBuffer, uint32_t imageIndex) -> ::vk::Result;
        void createFrameBuffers(::vk::RenderPass& renderPass);
        void beginRenderPass(const ::vk::CommandBuffer& commandBuffer, const ::vk::RenderPass& renderPass,
                             uint32_t imageIndex);
        [[nodiscard]] auto extentAspectRation() const -> float {
            return static_cast<float>(swapchainInfo.extent2D.width) / static_cast<float>(swapchainInfo.extent2D.width);
        }
        [[nodiscard]] auto compareFormats(const Swapchain& compareSwapchain) const -> bool {
            return depthFormat == compareSwapchain.depthFormat &&
                   swapchainInfo.formatKHR.format == compareSwapchain.swapchainInfo.formatKHR.format;
        }
        ~Swapchain();
        Swapchain(core::Device& device_, int width, int height, ::vk::SampleCountFlagBits sampleCount,
                  ::std::shared_ptr<Swapchain> oldSwapchain = nullptr);
        Swapchain(const Swapchain&) = delete;
        auto operator=(const Swapchain&) -> Swapchain& = delete;
        static constexpr int MAX_FRAME_IN_FLIGHT = 2;

    private:
        ::vk::SwapchainKHR swapchain;
        uint32_t currentFrame = 0;
        ::vk::SampleCountFlagBits sampleCount_;
        uint32_t mipLevels = 1;
        SwapchainInfo swapchainInfo;
        ::std::vector<::vk::Fence> inFlightFences;
        ::std::vector<::vk::Semaphore> imageAvailableSemaphores;
        ::std::vector<::vk::Semaphore> renderFinishSemaphores;

        // swapchain frame buffers
        ::std::vector<::vk::Framebuffer> frameBuffers;

        // swapchain image view
        ::std::vector<::vk::Image> images;
        ::std::vector<::vk::ImageView> imageViews;
        // depth image view
        ::std::vector<::vk::Image> depthImages;
        ::std::vector<::vk::DeviceMemory> depthImageMemorys;
        ::std::vector<::vk::ImageView> depthImageViews;

        // color image view
        ::std::vector<::vk::Image> colorImages;
        ::std::vector<::vk::ImageView> colorImageViews;
        ::std::vector<::vk::DeviceMemory> colorImageMemories;

        ::vk::Format depthFormat;
        static auto chooseSwapPresentMode(const ::std::vector<::vk::PresentModeKHR>& availablePresentModes)
            -> ::vk::PresentModeKHR;
        auto findDepthFormat() -> ::vk::Format;

        core::Device& device_;
        void init(int width, int height, ::std::shared_ptr<Swapchain>& oldSwapchain);
        void getImages();
        void createImageViews();
        void createImageFrame();
        void createFences();
        void createSemaphores();
        void createColorResources();
        void createDepthResources();
        void querySwapchainInfo(int width, int height);
};

}  // namespace g

#endif
