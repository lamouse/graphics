#include "g_swapchain.hpp"

namespace g
{

Swapchain::Swapchain(::std::shared_ptr<Device> device, int width, int height):device(device)
{
    querySwapchainInfo(width, height);
    vk::SwapchainCreateInfoKHR createInfo;
    //裁切
    createInfo.setClipped(true)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setSurface(device->getSurface())
            .setImageColorSpace(swapchainInfo.formatKHR.colorSpace)
            .setImageFormat(swapchainInfo.formatKHR.format)
            .setImageExtent(swapchainInfo.extent2D)
            .setMinImageCount(swapchainInfo.imageCount)
            .setPresentMode(swapchainInfo.present);
    auto& queueIndices = device->queueFamilyIndices;
    if(queueIndices.graphicsQueue.value() == queueIndices.presentQueue.value()){
        createInfo.setQueueFamilyIndexCount(queueIndices.graphicsQueue.value())
                    .setImageSharingMode(::vk::SharingMode::eExclusive);
    }else{
        ::std::array indices = {queueIndices.graphicsQueue.value(), queueIndices.presentQueue.value()};
        createInfo.setQueueFamilyIndices(indices)
                .setImageSharingMode(::vk::SharingMode::eConcurrent);
    }

    swapchain = device->getVKDevice().createSwapchainKHR(createInfo);

    createImageViews();
}

void Swapchain::querySwapchainInfo(int width, int height)
{
    auto& phyDevice = device->getPhysicalDevice();
    auto& surface = device->getSurface();
    auto  formats = phyDevice.getSurfaceFormatsKHR(surface);
    auto format = ::std::find_if(formats.begin(), formats.end(), [](auto format){
        return format.format == vk::Format::eB8G8R8A8Srgb && 
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    if(format != formats.end()){
        swapchainInfo.formatKHR = *format;
    } else {
        throw ::std::runtime_error("get image format error");
    }

    auto  cpabilities = phyDevice.getSurfaceCapabilitiesKHR(surface);
    swapchainInfo.imageCount = ::std::clamp<uint32_t>(2, cpabilities.minImageCount, cpabilities.maxImageCount);

    swapchainInfo.extent2D.width = ::std::clamp<uint32_t>(width, cpabilities.minImageExtent.width, cpabilities.maxImageExtent.width);
    swapchainInfo.extent2D.height = ::std::clamp<uint32_t>(height, cpabilities.minImageExtent.height, cpabilities.maxImageExtent.height);

    swapchainInfo.transForm = cpabilities.currentTransform;

    auto presents = phyDevice.getSurfacePresentModesKHR(surface);

    auto present = ::std::find_if(presents.begin(), presents.end(), [](auto present){
        return present == vk::PresentModeKHR::eMailbox;
    });
    if(present != presents.end()){
        swapchainInfo.present = *present;
    }
}

Swapchain::~Swapchain()
{
    for(auto& view : imageViews){
        device->getVKDevice().destroyImageView(view);
    }

    for(auto& image : images){
        device->getVKDevice().destroyImage(image);
    }
    
    device->getVKDevice().destroySwapchainKHR(swapchain);
}

void Swapchain::getImages()
{
    images = device->getVKDevice().getSwapchainImagesKHR(swapchain);
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
            .setBaseArrayLayer(VK_IMAGE_VIEW_TYPE_2D)
            .setLayerCount(1)
            .setAspectMask(vk::ImageAspectFlagBits::eColor);
        createImageViewInfo.setImage(images[i])
                            .setViewType(vk::ImageViewType::e2D)
                            .setComponents(mapping)
                            .setSubresourceRange(range);
        imageViews[i] = device->getVKDevice().createImageView(createImageViewInfo);
    }
}

}