#include "g_device.hpp"
#include <algorithm>
namespace g{

::std::unique_ptr<Device> Device::instance = nullptr;

Device::Device(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc)
{   
    createInstance(instanceExtends);
    vkSurfaceKHR = createFunc(vkInstance);
    pickupPhyiscalDevice();
    queryQueueFamilyIndices();
    createDevice();
    getQueues();
    initCmdPool();
}

void Device::init(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc)
{
    instance.reset(new Device(instanceExtends, createFunc));
}
void Device::quit()
{
    instance.reset();
}
Device::~Device()
{
    device.destroyCommandPool(cmdPool_);
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


void Device::createBuffer(::vk::DeviceSize size, ::vk::BufferUsageFlags usgae, ::vk::MemoryPropertyFlags properties, 
                                                    ::vk::Buffer& buffer, ::vk::DeviceMemory& bufferMemory)
{
    ::vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(size)
            .setUsage(usgae)
            .setSharingMode(::vk::SharingMode::eExclusive);
    buffer = device.createBuffer(bufferInfo);
    ::vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(buffer);
    ::vk::MemoryAllocateInfo allcoInfo;
    allcoInfo.setAllocationSize(memoryRequirements.size)
            .setMemoryTypeIndex(findMemoryType(memoryRequirements.memoryTypeBits, properties));
    bufferMemory = device.allocateMemory(allcoInfo);
    device.bindBufferMemory(buffer, bufferMemory, 0);
}


void Device::getQueues()
{
    graphicsQueue = device.getQueue(queueFamilyIndices.graphicsQueue.value(), 0);
    presentQueue = device.getQueue(queueFamilyIndices.presentQueue.value(), 0);
}

void Device::initCmdPool()
{
    ::vk::CommandPoolCreateInfo createInfo;
    uint32_t queueFamilyIndex = queueFamilyIndices.presentQueue.value();
    createInfo.setQueueFamilyIndex(queueFamilyIndex)
                .setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
                        ::vk::CommandPoolCreateFlagBits::eTransient);
    cmdPool_ =  device.createCommandPool(createInfo);
}

void Device::excuteCmd(RecordCmdFunc func)
{
    ::vk::CommandBufferAllocateInfo allocInfo(cmdPool_, ::vk::CommandBufferLevel::ePrimary, 1);
    auto commanBuffer = device.allocateCommandBuffers(allocInfo)[0];
    ::vk::CommandBufferBeginInfo beginInfo(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commanBuffer.begin(beginInfo);
        if (func) func(commanBuffer);
    commanBuffer.end();
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(commanBuffer);
    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
    device.waitIdle();
    device.freeCommandBuffers(cmdPool_, commanBuffer);
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

}
