#ifndef G_SWAPCHAIN_HPP
#define G_SWAPCHAIN_HPP
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
    void createImageFrame();
    ::vk::Framebuffer getFrameBuffer(int index){return frameBuffers[index];}
    static void init(int width, int height);
    static void quit();
    int getImageCount(){return images.size();}
    void frameBufferResize(int width, int height);
    static Swapchain& getInstance(){return *instance;}
    ~Swapchain();
private:
    SwapchainInfo swapchainInfo;
    ::std::vector<::vk::Image> images;
    ::std::vector<::vk::ImageView> imageViews;
    ::std::vector<::vk::Framebuffer> frameBuffers;
    ::vk::PresentModeKHR chooseSwapPresentMode(const ::std::vector<::vk::PresentModeKHR>& availablePresentModes);
    void getImages();
    void createImageViews();
    void createFrameBuffers();
    void recreateSwapchain();
    void querySwapchainInfo(int width, int height);
    Swapchain(int width, int height);
    static ::std::unique_ptr<Swapchain> instance;
    int width;
    int height;
};




}


#endif