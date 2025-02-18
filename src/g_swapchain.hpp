#ifndef G_SWAPCHAIN_HPP
#define G_SWAPCHAIN_HPP
#include <memory>
#include <vulkan/vulkan.hpp>

namespace g {

class Swapchain {
    public:
        [[nodiscard]] auto getSwapchainColorFormat() const { return imageFormat; };
        [[nodiscard]] auto getSwapchainDepthFormat() const { return depthFormat; }

        auto acquireNextImage() -> ::vk::ResultValue<uint32_t>;
        auto submitCommand(::vk::CommandBuffer& commandBuffer, uint32_t imageIndex) -> ::vk::Result;
        void createFrameBuffers(const ::vk::RenderPass& renderPass);
        void beginRenderPass(const ::vk::CommandBuffer& commandBuffer,
                             const ::vk::RenderPass& renderPass, uint32_t imageIndex) const;
        [[nodiscard]] auto extentAspectRation() const -> float {
            return static_cast<float>(extent_.width) / static_cast<float>(extent_.height);
        }
        [[nodiscard]] auto compareFormats(const Swapchain& compareSwapchain) const -> bool {
            return depthFormat == compareSwapchain.depthFormat &&
                   imageFormat == compareSwapchain.imageFormat;
        }
        ~Swapchain();
        Swapchain(int width, int height, ::vk::SampleCountFlagBits sampleCount,
                  ::std::shared_ptr<Swapchain> oldSwapchain = nullptr);
        Swapchain(const Swapchain&) = delete;
        Swapchain(Swapchain&&) = delete;
        auto operator=(Swapchain&&) -> Swapchain& = delete;
        auto operator=(const Swapchain&) -> Swapchain& = delete;
        static constexpr int MAX_FRAME_IN_FLIGHT = 2;
        static constexpr ::vk::Format DEFAULT_COLOR_FORMAT = ::vk::Format::eR8G8B8A8Unorm;
        static constexpr ::vk::ColorSpaceKHR DEFAULT_COLOR_SPACE =
            ::vk::ColorSpaceKHR::eSrgbNonlinear;

    private:
        ::vk::SwapchainKHR swapchain;
        uint32_t currentFrame = 0;
        ::vk::SampleCountFlagBits sampleCount_;
        uint32_t mipLevels = 1;
        // SwapchainInfo swapchainInfo;
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
        ::std::vector<::vk::DeviceMemory> depthImageMemories_;
        ::std::vector<::vk::ImageView> depthImageViews;

        // color image view
        ::std::vector<::vk::Image> colorImages;
        ::std::vector<::vk::ImageView> colorImageViews;
        ::std::vector<::vk::DeviceMemory> colorImageMemories;

        // swapchain formats
        ::vk::Format depthFormat;
        ::vk::Format imageFormat;

        ::vk::Extent2D extent_;

        static auto chooseSwapPresentMode(
            const ::std::vector<::vk::PresentModeKHR>& availablePresentModes)
            -> ::vk::PresentModeKHR;
        static auto chooseSwapSurfaceFormat(
            const ::std::vector<::vk::SurfaceFormatKHR>& availableFormats)
            -> ::vk::SurfaceFormatKHR;
        static auto chooseSwapExtent(const ::vk::SurfaceCapabilitiesKHR& capabilities, int width,
                                     int height) -> ::vk::Extent2D;
        [[nodiscard]] auto findDepthFormat() const -> ::vk::Format;

        void init(int width, int height, ::std::shared_ptr<Swapchain>& oldSwapchain);
        void getImages();
        void createImageViews();
        void createImageFrame();
        void createFences();
        void createSemaphores();
        void createColorResources();
        void createDepthResources();
};

}  // namespace g

#endif
