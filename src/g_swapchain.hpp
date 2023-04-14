#ifndef G_SWAPCHAIN_HPP
#define G_SWAPCHAIN_HPP
#include "core/device.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>

namespace g{

class Swapchain
{
public:
    struct SwapchainInfo{
        ::vk::Extent2D extent2D;
        ::vk::Extent3D extent3D;
        uint32_t imageCount;
        ::vk::SurfaceFormatKHR formatKHR;
        ::vk::SurfaceTransformFlagBitsKHR transForm;
        ::vk::PresentModeKHR presentMode;
    };
    ::vk::SwapchainKHR getSwapchain()const {return swapchain;}
    SwapchainInfo getSwapchainInfo()const {return swapchainInfo;}
    ::vk::Format getSwapchainColorFormat()const {return swapchainInfo.formatKHR.format;}
    ::vk::Format getSwapchainDepthFormat()const {return depthFormat;}
    ::vk::Framebuffer getFrameBuffer(int index){return swapchainFrameBuffers[index];}

    int getImageCount(){return images.size();}
    ::vk::ResultValue<uint32_t> acquireNextImage();
    ::vk::Result submitCommand(::vk::CommandBuffer& commandBuffer, uint32_t imageIndex);
    void createFrameBuffers(::vk::RenderPass& renderPass);
    void beginRenderPass(::vk::CommandBuffer& commandBuffer, uint32_t imageIndex, ::vk::RenderPass& renderPass);
    float extentAspectRation()
    {return static_cast<float>(swapchainInfo.extent2D.width)/ static_cast<float>(swapchainInfo.extent2D.width);}
    bool compareFormats(const Swapchain& swapchain) const {
        return depthFormat == swapchain.depthFormat &&
        swapchainInfo.formatKHR.format == swapchain.swapchainInfo.formatKHR.format;
    }
    ~Swapchain();
    Swapchain(core::Device& device_, int width, int height, ::vk::SampleCountFlagBits sampleCount, ::std::shared_ptr<Swapchain> oldSwapchain = nullptr);
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
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
    ::vk::PresentModeKHR chooseSwapPresentMode(const ::std::vector<::vk::PresentModeKHR>& availablePresentModes);
    ::vk::Format findDepthFormat();
    ::std::shared_ptr<Swapchain> oldSwapchain_;

    core::Device& device_;
    void init(int width, int height);
    void getImages();
    void createImageViews();
    void createImageFrame();
    void createFances();
    void createsemphores();
    void createColorResources();
    void createDepthResources();
    void querySwapchainInfo(int width, int height);
};

}


#endif
