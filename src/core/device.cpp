#include "device.hpp"
#include <algorithm>
#include <stdint.h>
#include <set>
namespace core{

::std::unique_ptr<Device> Device::instance = nullptr;

Device::Device(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc)
{   
    createInstance(instanceExtends);
    vkSurfaceKHR = createFunc(vkInstance);
    pickupPhyiscalDevice();
    createDevice();
    getQueues();
    initCmdPool();
}

Device::~Device()
{
    device_.destroyCommandPool(cmdPool_);
    device_.destroy();
    ::vkDestroySurfaceKHR(vkInstance, vkSurfaceKHR, nullptr);
    vkInstance.destroy();
}

void Device::pickupPhyiscalDevice()
{
    auto phyDevices = vkInstance.enumeratePhysicalDevices();


    auto findPhyDevice = ::std::find_if(phyDevices.begin(), phyDevices.end(), [this](auto& device){
            return isDeviceSuitable(device);
    });

    if(findPhyDevice != phyDevices.end())
    {
            phyDevice = *findPhyDevice;
            msaaSamples = getMaxUsableSampleCount();
            queueFamilyIndices = queryQueueFamilyIndices(phyDevice);
    }else 
    {
        throw ::std::runtime_error("failed to find a suitable GPU!");
    }
}

void Device::createDevice()
{
    ::vk::DeviceCreateInfo createInfo;
    ::std::vector<::vk::DeviceQueueCreateInfo> queueInfos;
    
    uint32_t queueCount = 1;
    float priorities = 0.5;
    ::vk::DeviceQueueCreateInfo queueInfo;
    queueInfo.setQueueCount(1)
            .setQueueFamilyIndex(queueFamilyIndices.graphicsQueue.value())
            .setQueuePriorities(priorities);
    queueInfos.push_back(::std::move(queueInfo));

    if(queueFamilyIndices.graphicsQueue.value() != queueFamilyIndices.presentQueue.value()){
        ::vk::DeviceQueueCreateInfo queueInfo;
        queueInfo.setQueueCount(queueCount)
                .setQueueFamilyIndex(queueFamilyIndices.presentQueue.value())
                .setQueuePriorities(priorities);
        queueInfos.push_back(::std::move(queueInfo));
    }

    ::vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    createInfo.setQueueCreateInfos(queueInfos)
                .setPEnabledExtensionNames(deviceExtensions)
                .setPEnabledFeatures(&deviceFeatures);

    device_ = phyDevice.createDevice(createInfo);
}
    
Device::QueueFamilyIndices Device::queryQueueFamilyIndices(::vk::PhysicalDevice device)
{
    auto properties = device.getQueueFamilyProperties();

    
    auto pos = ::std::find_if(properties.begin(), properties.end(), [](const auto & property){
        return property.queueFlags | vk::QueueFlagBits::eGraphics;
    });

    QueueFamilyIndices indices;
    if(pos != properties.end()){
        indices.graphicsQueue = pos - properties.begin();
        if(device.getSurfaceSupportKHR(indices.graphicsQueue.value(), vkSurfaceKHR)){
            indices.presentQueue = indices.graphicsQueue;
        }
    }
    return indices;

}


void Device::createBuffer(::vk::DeviceSize size, ::vk::BufferUsageFlags usgae, ::vk::MemoryPropertyFlags properties, 
                                                    ::vk::Buffer& buffer, ::vk::DeviceMemory& bufferMemory)
{
    ::vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(size)
            .setUsage(usgae)
            .setSharingMode(::vk::SharingMode::eExclusive);
    buffer = device_.createBuffer(bufferInfo);
    ::vk::MemoryRequirements memoryRequirements = device_.getBufferMemoryRequirements(buffer);
    ::vk::MemoryAllocateInfo allcoInfo;
    allcoInfo.setAllocationSize(memoryRequirements.size)
            .setMemoryTypeIndex(findMemoryType(memoryRequirements.memoryTypeBits, properties));
    bufferMemory = device_.allocateMemory(allcoInfo);
    device_.bindBufferMemory(buffer, bufferMemory, 0);
}


void Device::getQueues()
{
    graphicsQueue = device_.getQueue(queueFamilyIndices.graphicsQueue.value(), 0);
    presentQueue = device_.getQueue(queueFamilyIndices.presentQueue.value(), 0);
}

void Device::initCmdPool()
{
    ::vk::CommandPoolCreateInfo createInfo;
    uint32_t queueFamilyIndex = queueFamilyIndices.presentQueue.value();
    createInfo.setQueueFamilyIndex(queueFamilyIndex)
                .setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
                        ::vk::CommandPoolCreateFlagBits::eTransient);
    cmdPool_ =  device_.createCommandPool(createInfo);
}

void Device::excuteCmd(RecordCmdFunc func)
{
    ::vk::CommandBufferAllocateInfo allocInfo(cmdPool_, ::vk::CommandBufferLevel::ePrimary, 1);
    auto commanBuffer = device_.allocateCommandBuffers(allocInfo)[0];
    ::vk::CommandBufferBeginInfo beginInfo(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commanBuffer.begin(beginInfo);
        if (func) func(commanBuffer);
    commanBuffer.end();
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(commanBuffer);
    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
    device_.waitIdle();
    device_.freeCommandBuffers(cmdPool_, commanBuffer);
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
    return device_;
}

uint32_t Device::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    ::vk::PhysicalDeviceMemoryProperties memProperties = phyDevice.getMemoryProperties();;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Device::createInstance(const std::vector<const char*>& instanceExtends)
{
    ::std::vector<const char*> layers{"VK_LAYER_KHRONOS_validation"};
    ::std::vector<const char*> externs = {instanceExtends.begin(), instanceExtends.end()};
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    externs.push_back("VK_KHR_portability_enumeration");
    ::vk::InstanceCreateFlags flags{VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR};
#endif
 
    ::vk::InstanceCreateInfo createInfo;
    ::vk::ApplicationInfo appInfo;
     appInfo.setApiVersion(VK_API_VERSION_1_3)
            .setPApplicationName("graphics")
            .setPEngineName("engin")
            .setApplicationVersion(VK_MAKE_VERSION(0,0, 1));
     createInfo.setPEnabledLayerNames(layers)
                .setPApplicationInfo(&appInfo)
#if defined(VK_USE_PLATFORM_MACOS_MVK)
                .setFlags(flags)
#endif
                .setPEnabledExtensionNames(externs);
    vkInstance = ::vk::createInstance(createInfo);
}

void Device::createImage(uint32_t width, uint32_t height, uint32_t mipLevels ,::vk::Format format, ::vk::SampleCountFlagBits numSamples, ::vk::ImageTiling tiling, 
                        ::vk::ImageUsageFlags usage, ::vk::MemoryPropertyFlags properties, ::vk::Image& image, ::vk::DeviceMemory& imageMemory)
{
    ::vk::ImageCreateInfo imageInfo;
    imageInfo.setImageType(::vk::ImageType::e2D)
            .setExtent(::vk::Extent3D{{width, height}, 1})
            .setMipLevels(mipLevels)
            .setArrayLayers(1)
            .setTiling(tiling)
            .setFormat(format)
            .setInitialLayout(::vk::ImageLayout::eUndefined)
            .setUsage(usage)
            .setSharingMode(::vk::SharingMode::eExclusive)
            .setSamples(numSamples);
    
   image = device_.createImage(imageInfo);
    ::vk::MemoryRequirements memRequirements = device_.getImageMemoryRequirements(image);
    ::vk::MemoryAllocateInfo allocInfo;
    allocInfo.setAllocationSize(memRequirements.size)
                .setMemoryTypeIndex(findMemoryType(memRequirements.memoryTypeBits, properties));
    imageMemory = device_.allocateMemory(allocInfo);
    device_.bindImageMemory(image, imageMemory, 0);
}

::vk::ImageView Device::createImageView(::vk::Image image, ::vk::Format format, ::vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    ::vk::ImageViewCreateInfo viewInfo{};
    ::vk::ImageSubresourceRange imageSubresource;
    imageSubresource.setAspectMask(aspectFlags)
                    .setBaseMipLevel(0)
                    .setLevelCount(mipLevels)
                    .setLayerCount(1)
                    .setBaseArrayLayer(0);
    viewInfo.setImage(image)
            .setViewType(::vk::ImageViewType::e2D)
            .setFormat(format)
            .setSubresourceRange(imageSubresource);

    return device_.createImageView(viewInfo);
}

float Device::getMaxAnisotropy()
{
    ::vk::PhysicalDeviceProperties properties = phyDevice.getProperties();
    return properties.limits.maxSamplerAnisotropy;
}

::vk::SampleCountFlagBits Device::getMaxUsableSampleCount()
{
    vk::PhysicalDeviceProperties physicalDevice = phyDevice.getProperties();
    ::vk::SampleCountFlags counts = physicalDevice.limits.framebufferColorSampleCounts & physicalDevice.limits.framebufferDepthSampleCounts;
    if(counts & ::vk::SampleCountFlagBits::e64){return ::vk::SampleCountFlagBits::e64;}
    if(counts & ::vk::SampleCountFlagBits::e32){return ::vk::SampleCountFlagBits::e32;}
    if(counts & ::vk::SampleCountFlagBits::e16){return ::vk::SampleCountFlagBits::e16;}
    if(counts & ::vk::SampleCountFlagBits::e8){return ::vk::SampleCountFlagBits::e8;}
    if(counts & ::vk::SampleCountFlagBits::e4){return ::vk::SampleCountFlagBits::e4;}
    if(counts & ::vk::SampleCountFlagBits::e2){return ::vk::SampleCountFlagBits::e2;}

    return ::vk::SampleCountFlagBits::e1;
}

bool Device::checkDeviceExtensionSupport(::vk::PhysicalDevice& checkDevice)
{
    std::vector<vk::ExtensionProperties> availableExtensions = checkDevice.enumerateDeviceExtensionProperties();
    ::std::set<::std::string> requireExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for(const auto& extension : availableExtensions)
    {
        requireExtensions.erase(extension.extensionName);
    }

    return requireExtensions.empty();
}

bool Device::isDeviceSuitable(::vk::PhysicalDevice& checkDevice)
{
    bool extensionsSupported = checkDeviceExtensionSupport(checkDevice);
    bool swapChainAdeqate = false;
    if(extensionsSupported)
    {
        SwapchainSupportDetails swapchainSupport = querySwapchainSupport(checkDevice);
        swapChainAdeqate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }
    ::vk::PhysicalDeviceFeatures deviceFeatures = checkDevice.getFeatures();
    auto indices = queryQueueFamilyIndices(checkDevice);
    return extensionsSupported && swapChainAdeqate && deviceFeatures.samplerAnisotropy && indices.isComplete();
}

SwapchainSupportDetails Device::querySwapchainSupport(::vk::PhysicalDevice device)
{
    SwapchainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(vkSurfaceKHR);
    details.formats = device.getSurfaceFormatsKHR(vkSurfaceKHR);
    details.presentModes = device.getSurfacePresentModesKHR(vkSurfaceKHR);
    return details;
}

}
