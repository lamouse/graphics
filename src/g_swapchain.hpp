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
        [[nodiscard]] auto getSwapchain() const -> ::vk::SwapchainKHR {
            return swapchain;
        }
        [[nodiscard]] auto getSwapchainInfo() const -> SwapchainInfo {
            return swapchainInfo;
        }
        [[nodiscard]] auto getSwapchainColorFormat() const -> ::vk::Format;
        [[nodiscard]] auto getSwapchainDepthFormat() const -> ::vk::Format {
            return depthFormat;
        }
        auto getFrameBuffer(int index) -> ::vk::Framebuffer {
            return swapchainFrameBuffers[index];
        }

        auto getImageCount() { return images.size(); }
        auto acquireNextImage() -> ::vk::ResultValue<uint32_t>;
        auto submitCommand(::vk::CommandBuffer& commandBuffer,
                           uint32_t imageIndex) -> ::vk::Result;
        void createFrameBuffers(::vk::RenderPass& renderPass);
        void beginRenderPass(::vk::CommandBuffer& commandBuffer,
                             ::vk::RenderPass& renderPass, uint32_t imageIndex);
        [[nodiscard]] auto extentAspectRation() const -> float {
            return static_cast<float>(swapchainInfo.extent2D.width) /
                   static_cast<float>(swapchainInfo.extent2D.width);
        }
        [[nodiscard]] auto compareFormats(const Swapchain& swapchain) const
            -> bool {
            return depthFormat == swapchain.depthFormat &&
                   swapchainInfo.formatKHR.format ==
                       swapchain.swapchainInfo.formatKHR.format;
        }
        ~Swapchain();
        Swapchain(core::Device& device_, int width, int height,
                  ::vk::SampleCountFlagBits sampleCount,
                  ::std::shared_ptr<Swapchain> oldSwapchain = nullptr);
        Swapchain(const Swapchain&) = delete;
        auto operator=(const Swapchain&) -> Swapchain& = delete;
        static constexpr int MAX_FRAME_IN_FLIGHT = 2;

    private:
        ::vk::SwapchainKHR swapchain;
        uint32_t currentFrame = 0;
        ::vk::SampleCountFlagBits sampleCount_;
        SwapchainInfo swapchainInfo;
        ::std::vector<::vk::Fence> inFlightFences;
        ::std::vector<::vk::Semaphore> imageAvailableSemaphores;
        ::std::vector<::vk::Semaphore> renderFinshSemaphores;
        // swapchain image view
        ::std::vector<::vk::Image> images;
        ::std::vector<::vk::ImageView> imageViews;
        ::std::vector<::vk::Framebuffer> swapchainFrameBuffers;
        // depth image view
        ::std::vector<::vk::Image> depthImages;
        ::std::vector<::vk::DeviceMemory> depthImageMemorys;
        ::std::vector<::vk::ImageView> depthImageViews;

        // color image view
        ::std::vector<::vk::Image> colorImages;
        ::std::vector<::vk::ImageView> colorImageViews;
        ::std::vector<::vk::DeviceMemory> colorImageMemorys;

        ::vk::Format depthFormat;
        static auto chooseSwapPresentMode(
            const ::std::vector<::vk::PresentModeKHR>& availablePresentModes)
            -> ::vk::PresentModeKHR;
        auto findDepthFormat() -> ::vk::Format;

        core::Device& device_;
        void init(int width, int height,
                  ::std::shared_ptr<Swapchain>& oldSwapchain);
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
