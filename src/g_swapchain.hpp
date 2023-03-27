#ifndef G_SWAPCHAIN_HPP
#define G_SWAPCHAIN_HPP
#include "g_device.hpp"
#include <stdint.h>
#include<vulkan/vulkan.hpp>
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
        ::vk::SurfaceTransformFlagsKHR transForm;
        ::vk::PresentModeKHR presentMode;
    };
    ::vk::SwapchainKHR swapchain;
    ::vk::SwapchainKHR getSwapchain()const {return swapchain;}
    SwapchainInfo getSwapchainInfo()const {return swapchainInfo;}
    ::vk::Framebuffer getFrameBuffer(int index){return frameBuffers[index];}

    int getImageCount(){return images.size();}
    ::vk::RenderPass getRenderPass(){return renderPass;};
    ::vk::ResultValue<uint32_t> acquireNextImage();
    ::vk::Result submitCommand(::vk::CommandBuffer& commandBuffer, uint32_t imageIndex);
    void beginRenderPass(::vk::CommandBuffer& commandBuffer, uint32_t imageIndex);
    float extentAspectRation()
    {return static_cast<float>(swapchainInfo.extent2D.width)/ static_cast<float>(swapchainInfo.extent2D.width);}
    bool compareFormats(const Swapchain& swapchain) const {
        return depthFormat == swapchain.depthFormat &&
        swapchainInfo.formatKHR.format == swapchain.swapchainInfo.formatKHR.format;
    }
    
    ~Swapchain();
    Swapchain(int width, int height);
    Swapchain(int width, int height, ::std::shared_ptr<Swapchain> oldSwapchain);
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    static constexpr int MAX_FRAME_IN_FLIGHT = 2;

private:

    uint32_t currentFrame = 0;
    SwapchainInfo swapchainInfo;
    ::std::vector<::vk::Fence> inFlightFences;
    ::std::vector<::vk::Semaphore> imageAvailableSemaphores;
    ::std::vector<::vk::Semaphore> renderFinshSemaphores;
    ::std::vector<::vk::Image> images;
    ::std::vector<::vk::ImageView> imageViews;
    ::std::vector<::vk::Framebuffer> frameBuffers;
    ::std::vector<VkImage> depthImages;
    ::std::vector<VkDeviceMemory> depthImageMemorys;
    ::std::vector<VkImageView> depthImageViews;
    ::vk::RenderPass renderPass;
    ::vk::Format depthFormat;
    ::vk::PresentModeKHR chooseSwapPresentMode(const ::std::vector<::vk::PresentModeKHR>& availablePresentModes);
    ::vk::Format findDepthFormat();
    ::std::shared_ptr<Swapchain> oldSwapchain;
    void init(int width, int height);
    void getImages();
    void createImageViews();
    void createFrameBuffers();
    void createImageFrame();
    void initRenderPass();
    void createFances();
    void createsemphores();
    void createDepthResources();
    void querySwapchainInfo(int width, int height);
};




}


#endif
