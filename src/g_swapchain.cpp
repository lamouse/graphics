#include "g_swapchain.hpp"
#include "g_device.hpp"
#include "g_render.hpp"
#include <iostream>
namespace g
{
::std::unique_ptr<Swapchain> Swapchain::instance = nullptr;

Swapchain::Swapchain( int width, int height):width(width), height(height)
{
    querySwapchainInfo(width, height);
    vk::SwapchainCreateInfoKHR createInfo;
    //裁切
    createInfo.setClipped(true)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setSurface(Device::getInstance().getSurface())
            .setImageColorSpace(swapchainInfo.formatKHR.colorSpace)
            .setImageFormat(swapchainInfo.formatKHR.format)
            .setImageExtent(swapchainInfo.extent2D)
            .setMinImageCount(swapchainInfo.imageCount)
            .setPresentMode(swapchainInfo.presentMode);
    auto& queueIndices = Device::getInstance().queueFamilyIndices;
    if(queueIndices.graphicsQueue.value() == queueIndices.presentQueue.value()){
        createInfo.setQueueFamilyIndexCount(queueIndices.graphicsQueue.value())
                    .setImageSharingMode(::vk::SharingMode::eExclusive);
    }else{
        ::std::array indices = {queueIndices.graphicsQueue.value(), queueIndices.presentQueue.value()};
        createInfo.setQueueFamilyIndices(indices)
                .setImageSharingMode(::vk::SharingMode::eConcurrent);
    }
    swapchain = Device::getInstance().getVKDevice().createSwapchainKHR(createInfo);
}

void Swapchain::createImageFrame()
{
    createImageViews();
    createFrameBuffers();
}

void Swapchain::frameBufferResize(int width, int height)
{
    this->width = width;
    this->height = height;
    void recordCommandBuffer();
    recreateSwapchain();
}

void Swapchain::recreateSwapchain()
{
    Device::getInstance().getVKDevice().waitIdle();
    instance = ::std::make_unique<Swapchain>(Swapchain(width, height));
}

void Swapchain::init(int width, int height)
{
    instance.reset(new Swapchain(width, height));
}
void Swapchain::quit()
{
    instance.reset();
}

void Swapchain::querySwapchainInfo(int width, int height)
{
    auto& phyDevice = Device::getInstance().getPhysicalDevice();
    auto& surface = Device::getInstance().getSurface();
    auto  formats = phyDevice.getSurfaceFormatsKHR(surface);
    auto format = ::std::find_if(formats.begin(), formats.end(), [](auto format){
        return format.format == vk::Format::eB8G8R8A8Srgb && 
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    if(format != formats.end()){
        swapchainInfo.formatKHR = *format;
    } else {
         swapchainInfo.formatKHR = formats[0];
    }

    auto  cpabilities = phyDevice.getSurfaceCapabilitiesKHR(surface);
    swapchainInfo.imageCount = ::std::clamp<uint32_t>(2, cpabilities.minImageCount, cpabilities.maxImageCount);

    swapchainInfo.extent2D.width = ::std::clamp<uint32_t>(width, cpabilities.minImageExtent.width, cpabilities.maxImageExtent.width);
    swapchainInfo.extent2D.height = ::std::clamp<uint32_t>(height, cpabilities.minImageExtent.height, cpabilities.maxImageExtent.height);

    swapchainInfo.transForm = cpabilities.currentTransform;

    auto presents = phyDevice.getSurfacePresentModesKHR (surface);

    swapchainInfo.presentMode = chooseSwapPresentMode(presents);
}
::vk::PresentModeKHR Swapchain::chooseSwapPresentMode(const ::std::vector<::vk::PresentModeKHR>& availablePresentModes)
{


    //使用最新的图像，画面延迟更低，gpu不会闲置，功耗高
    for(const auto& availablePresentMode : availablePresentModes)
    {
        if(availablePresentMode == ::vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }

    // 不进行垂直同步，使用最高的cpu个gpu性能，能达到最高的fps，但可能造成画面撕裂
    // for(const auto& availablePresentMode : availablePresentModes)
    // {
    //     if(availablePresentMode == ::vk::PresentModeKHR::eImmediate)
    //     {
    //         return availablePresentMode;
    //     }
    // }

    //先进先出，排队
    return ::vk::PresentModeKHR::eFifo;
}
Swapchain::~Swapchain()
{
    for(auto& frameBuffer : frameBuffers){
        Device::getInstance().getVKDevice().destroyFramebuffer(frameBuffer);
    }

    for(auto& view : imageViews){
        Device::getInstance().getVKDevice().destroyImageView(view);
    }
    
    Device::getInstance().getVKDevice().destroySwapchainKHR(swapchain);
}

void Swapchain::getImages()
{
    images = Device::getInstance().getVKDevice().getSwapchainImagesKHR(swapchain);
}

void Swapchain::createImageViews()
{
    getImages();
    imageViews.resize(images.size());
    for(int i = 0; i < images.size(); i++)
    {
        ::vk::ComponentMapping mapping;
        ::vk::ImageViewCreateInfo createImageViewInfo;
        ::vk::ImageSubresourceRange range;
        range.setBaseMipLevel(0)
            .setLevelCount(VK_REMAINING_MIP_LEVELS)
            .setBaseArrayLayer(VK_IMAGE_VIEW_TYPE_1D)
            .setLayerCount(1)
            .setAspectMask(vk::ImageAspectFlagBits::eColor);
        createImageViewInfo.setImage(images[i])
                            .setViewType(vk::ImageViewType::e2D)
                            .setComponents(mapping)
                            .setSubresourceRange(range)
                            .setFormat(swapchainInfo.formatKHR.format);
        imageViews[i] = Device::getInstance().getVKDevice().createImageView(createImageViewInfo);
    }
}

void Swapchain::createFrameBuffers()
{
    frameBuffers.resize(images.size());

    for(int i = 0; i < frameBuffers.size(); i++)
    {
        ::vk::FramebufferCreateInfo createInfo;
        createInfo.setAttachments(imageViews[i])
                .setWidth(width)
                .setHeight(height)
                .setRenderPass(RenderProcess::getInstance().getRenderPass())
                .setLayers(1);
        frameBuffers[i] = Device::getInstance().getVKDevice().createFramebuffer(createInfo);
    }
}
}
