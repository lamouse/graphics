#ifndef G_SWAPCHAIN_HPP
#define G_SWAPCHAIN_HPP
#include "g_device.hpp"
#include<vulkan/vulkan.hpp>
#include <memory>

namespace g{

class Swapchain
{
private:

    struct SwapchainInfo{
        ::vk::Extent2D extent2D;
        ::vk::Extent3D extent3D;
        uint32_t imageCount;
        ::vk::SurfaceFormatKHR formatKHR;
        ::vk::SurfaceTransformFlagsKHR transForm;
        ::vk::PresentModeKHR present;
    };
    ::std::shared_ptr<Device> device;
    SwapchainInfo swapchainInfo;
    ::vk::SwapchainKHR swapchain;
    void querySwapchainInfo(int width, int height);
public:
    Swapchain(::std::shared_ptr<Device> device, int width, int height);
    ~Swapchain();
};




}


#endif