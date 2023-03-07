#include "g_device.hpp"
#include <algorithm>
namespace g{
    
Device::Device(const ::vk::Instance& instance, CreateSurfaceFunc func):
                    vkInstance{instance}
{   
    vkSurfaceKHR = func(vkInstance);
    pickupPhyiscalDevice();
    queryQueueFamilyIndices();
    createDevice();
    getQueues();
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
    ::std::vector<const char*> extends{"VK_KHR_portability_subset", VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    float priorities = 1.0;

    ::vk::DeviceQueueCreateInfo queueInfo;
    queueInfo.setQueuePriorities(priorities)
            .setQueueCount(queueFamilyIndices.queueCount.value())
            .setQueueFamilyIndex(queueFamilyIndices.graphicsQueue.value());
    queueInfos.push_back(::std::move(queueInfo));

    if(queueFamilyIndices.graphicsQueue.value() != queueFamilyIndices.presentQueue.value()){
        ::vk::DeviceQueueCreateInfo queueInfo;
        queueInfo.setQueuePriorities(priorities)
                .setQueueCount(queueFamilyIndices.queueCount.value())
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
}

::vk::Instance& Device::getInstance()
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

::vk::Device& Device::getVKDevice()
{
    return device;
}

}