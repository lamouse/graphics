#include "g_device.hpp"
#include <algorithm>
namespace g{

::std::unique_ptr<Device> Device::instance = nullptr;

Device::Device(const ::vk::Instance& instance, ::vk::SurfaceKHR vkSurfaceKHR):
                    vkInstance{instance}, vkSurfaceKHR{vkSurfaceKHR}
{   
    pickupPhyiscalDevice();
    queryQueueFamilyIndices();
    createDevice();
    getQueues();
}

void Device::init(const ::vk::Instance& vkInstance, ::vk::SurfaceKHR vkSurfaceKHR)
{
    instance.reset(new Device(vkInstance, vkSurfaceKHR));
}
void Device::quit()
{
    instance.reset();
}
Device::~Device()
{
    device.destroy();
    ::vkDestroySurfaceKHR(vkInstance, vkSurfaceKHR, nullptr);
    vkInstance.destroy();
}

void Device::pickupPhyiscalDevice()
{
    auto phyDevices = vkInstance.enumeratePhysicalDevices();
    phyDevice = phyDevices[0];

}

void Device::createDevice()
{
    queryQueueFamilyIndices();
    ::vk::DeviceCreateInfo createInfo;
    ::std::vector<::vk::DeviceQueueCreateInfo> queueInfos;
    // "VK_KHR_portability_subset" macos
    
#if defined(VK_USE_PLATFORM_MACOS_MVK)
   ::std::vector<const char*> extends{VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"};
#else
    ::std::vector<const char*> extends{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#endif

    float priorities = 0.5;
    uint32_t queueCount = 1;
    ::vk::DeviceQueueCreateInfo queueInfo;
    queueInfo.setPQueuePriorities(&priorities)
            .setQueueCount(1)
            .setQueueFamilyIndex(queueFamilyIndices.graphicsQueue.value());
    queueInfos.push_back(::std::move(queueInfo));

    if(queueFamilyIndices.graphicsQueue.value() != queueFamilyIndices.presentQueue.value()){
        ::vk::DeviceQueueCreateInfo queueInfo;
        queueInfo.setPQueuePriorities(&priorities)
                .setQueueCount(queueCount)
                .setQueueFamilyIndex(queueFamilyIndices.presentQueue.value());
        queueInfos.push_back(::std::move(queueInfo));
    }


    createInfo.setQueueCreateInfos(queueInfos)
                .setPEnabledExtensionNames(extends);

    device = phyDevice.createDevice(createInfo);
}
    
void Device::queryQueueFamilyIndices()
{
    auto properties = phyDevice.getQueueFamilyProperties();

    
    auto pos = ::std::find_if(properties.begin(), properties.end(), [](const auto & property){
        return property.queueFlags | vk::QueueFlagBits::eGraphics;
    });


    if(pos != properties.end()){
        queueFamilyIndices.graphicsQueue = pos - properties.begin();
        queueFamilyIndices.queueCount = pos->queueCount;
        if(phyDevice.getSurfaceSupportKHR(queueFamilyIndices.graphicsQueue.value(), vkSurfaceKHR)){
            queueFamilyIndices.presentQueue = queueFamilyIndices.graphicsQueue;
        }

        if(!queueFamilyIndices.isComplete()){
            throw ::std::runtime_error("queue family complete fail");
        }
    }

}

void Device::getQueues()
{
    graphicsQueue = device.getQueue(queueFamilyIndices.graphicsQueue.value(), 0);
    presentQueue = device.getQueue(queueFamilyIndices.presentQueue.value(), 0);
}

::vk::Instance& Device::getVKInstance()
{
    return vkInstance;
}

::vk::SurfaceKHR& Device::getSurface()
{
    return vkSurfaceKHR;
}

::vk::PhysicalDevice& Device::getPhysicalDevice()
{
    return phyDevice;
}

::vk::Queue& Device::getGraphicsQueue()
{
    return graphicsQueue;
}

::vk::Queue& Device::getPresentQueue()
{
    return presentQueue;
}

::vk::Format Device::findSupportedFormat(
    const std::vector<::vk::Format> &candidates, ::vk::ImageTiling tiling, ::vk::FormatFeatureFlags features) {
  for (::vk::Format format : candidates) {
    ::vk::FormatProperties props = phyDevice.getFormatProperties(format);

    if (tiling == ::vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (
        tiling == ::vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("failed to find supported format!");
}

::vk::Device& Device::getVKDevice()
{
    return device;
}

uint32_t Device::findMemoryType(uint32_t typeFilter, ::vk::MemoryPropertyFlags properties) {
  ::vk::PhysicalDeviceMemoryProperties memProperties = phyDevice.getMemoryProperties();
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
}
}
