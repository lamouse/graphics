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
        ::vk::PresentModeKHR present;
    };
    ::vk::SwapchainKHR getSwapchain()const {return swapchain;}
    SwapchainInfo getSwapchainInfo()const {return swapchainInfo;}
    static void init(int width, int height);
    static void quit();
    static Swapchain& getInstance(){return *instance;}

    ~Swapchain();
private:
    SwapchainInfo swapchainInfo;
    ::vk::SwapchainKHR swapchain;
    ::std::vector<::vk::Image> images;
    ::std::vector<::vk::ImageView> imageViews;
    void getImages();
    void createImageViews();
    void querySwapchainInfo(int width, int height);
    Swapchain(int width, int height);
    static ::std::unique_ptr<Swapchain> instance;
};




}


#endif