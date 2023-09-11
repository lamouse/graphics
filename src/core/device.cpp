#include "device.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <set>

namespace core {
namespace {
VKAPI_ATTR auto VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/)
    -> VkBool32 {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
            spdlog::warn("validation layer: {}", pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
            spdlog::debug("validation layer: {}", pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
            spdlog::error("validation layer: {}", pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
            break;
    }

    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.setMessageSeverity(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    createInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                              vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                              vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
    createInfo.pfnUserCallback = debugCallback;
}

auto CreateDebugUtilsMessengerEXT(::vk::Instance instance, const ::vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                  const ::vk::AllocationCallbacks* pAllocator,
                                  ::vk::DebugUtilsMessengerEXT* pDebugMessenger) -> ::vk::Result {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)instance.getProcAddr("vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return (::vk::Result)func(instance, (VkDebugUtilsMessengerCreateInfoEXT*)pCreateInfo,
                                  (const VkAllocationCallbacks*)pAllocator, (VkDebugUtilsMessengerEXT*)pDebugMessenger);
    }
    return ::vk::Result::eErrorExtensionNotPresent;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void setupDebugMessenger(::vk::Instance& instance, ::vk::DebugUtilsMessengerEXT& debugMessenger) {
    ::vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != ::vk::Result::eSuccess) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

auto checkValidationLayerSupport() -> bool {
    std::vector<::vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char* layerName : validationLayers) {
        auto layerProperty = ::std::find_if(
            availableLayers.begin(), availableLayers.end(),
            [&layerName](const auto& layerProperties) { return strcmp(layerName, layerProperties.layerName) == 0; });
        if (layerProperty == availableLayers.end()) {
            return false;
        }
    }

    return true;
}

}  // namespace

Device::Device(const std::vector<const char*>& instanceExtends, const CreateSurfaceFunc& createFunc,
               bool enableValidationLayers) {
    enableValidationLayers_ = enableValidationLayers;
    createInstance(instanceExtends);
    vkSurfaceKHR = createFunc(vkInstance);
    pickupPhysicalDevice();
    createLogicalDevice();
    getQueues();
    initCmdPool();
}

Device::~Device() {
    device_.destroyCommandPool(cmdPool_);
    device_.destroy();
    vkInstance.destroySurfaceKHR(vkSurfaceKHR);
    if (enableValidationLayers_) {
        DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger_, nullptr);
    }
    vkInstance.destroy();
}

void Device::createInstance(const std::vector<const char*>& instanceExtends) {
    if (enableValidationLayers_ && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    ::vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName("graphics")
        .setPEngineName("engin")
        .setApiVersion(VK_API_VERSION_1_0)
        .setEngineVersion(VK_MAKE_VERSION(1, 3, 0))
        .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));

    ::vk::InstanceCreateInfo createInfo;

    if (enableValidationLayers_) {
        createInfo.setPEnabledLayerNames(validationLayers);
        ::vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }

    createInfo.setPApplicationInfo(&appInfo)
        .setFlags(::vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR)
        .setPEnabledExtensionNames(instanceExtends);
    vkInstance = ::vk::createInstance(createInfo);
    if (enableValidationLayers_) {
        setupDebugMessenger(vkInstance, debugMessenger_);
    }
}

void Device::pickupPhysicalDevice() {
    auto phyDevices = vkInstance.enumeratePhysicalDevices();
    if (phyDevices.empty()) {
        throw ::std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    auto findPhyDevice =
        ::std::find_if(phyDevices.begin(), phyDevices.end(), [this](auto& device) { return isDeviceSuitable(device); });

    if (findPhyDevice != phyDevices.end()) {
        phyDevice = *findPhyDevice;
        maxMsaaSamples_ = getMaxUsableSampleCount();
        queueFamilyIndices = queryQueueFamilyIndices(phyDevice);
    } else {
        throw ::std::runtime_error("failed to find a suitable GPU!");
    }
}

void Device::createLogicalDevice() {
    ::std::vector<::vk::DeviceQueueCreateInfo> queueInfos;
    ::std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsQueue.value(),
                                                queueFamilyIndices.presentQueue.value()};
    float priorities = 1.f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        ::vk::DeviceQueueCreateInfo queueInfo({}, queueFamily, 1, &priorities);
        queueInfos.push_back(queueInfo);
    }

    ::vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.setSamplerAnisotropy(VK_TRUE);

    ::vk::DeviceCreateInfo createInfo;
    createInfo.setQueueCreateInfos(queueInfos)
        .setPEnabledExtensionNames(deviceExtensions)
        .setPEnabledFeatures(&deviceFeatures);
    if (enableValidationLayers_) {
        createInfo.setPEnabledLayerNames(validationLayers);
    }

    device_ = phyDevice.createDevice(createInfo);
}

auto Device::queryQueueFamilyIndices(::vk::PhysicalDevice device) -> Device::QueueFamilyIndices {
    auto properties = device.getQueueFamilyProperties();
    int index = 0;
    auto pos = ::std::find_if(properties.begin(), properties.end(), [&](const auto& property) {
        if ((property.queueFlags | ::vk::QueueFlagBits::eGraphics) &&
            (property.queueFlags | ::vk::QueueFlagBits::eCompute)) {
            if (device.getSurfaceSupportKHR(index, vkSurfaceKHR)) {
                return true;
            }
        }
        index++;
        return false;
    });

    QueueFamilyIndices indices;
    if (pos != properties.end()) {
        indices.graphicsQueue = static_cast<uint32_t>(pos - properties.begin());
        indices.presentQueue = indices.graphicsQueue;
        indices.computeQueue = indices.graphicsQueue;
    }
    return indices;
}

void Device::createBuffer(::vk::DeviceSize size, ::vk::BufferUsageFlags usage, ::vk::MemoryPropertyFlags properties,
                          ::vk::Buffer& buffer, ::vk::DeviceMemory& bufferMemory) {
    ::vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(size).setUsage(usage).setSharingMode(::vk::SharingMode::eExclusive);
    buffer = device_.createBuffer(bufferInfo);
    ::vk::MemoryRequirements memoryRequirements = device_.getBufferMemoryRequirements(buffer);
    ::vk::MemoryAllocateInfo allocateInfo;
    allocateInfo.setAllocationSize(memoryRequirements.size)
        .setMemoryTypeIndex(findMemoryType(memoryRequirements.memoryTypeBits, properties));
    bufferMemory = device_.allocateMemory(allocateInfo);
    device_.bindBufferMemory(buffer, bufferMemory, 0);
}

void Device::getQueues() {
    graphicsQueue = device_.getQueue(queueFamilyIndices.graphicsQueue.value(), 0);
    presentQueue = device_.getQueue(queueFamilyIndices.presentQueue.value(), 0);
    computeQueue = device_.getQueue(queueFamilyIndices.computeQueue.value(), 0);
}

void Device::initCmdPool() {
    ::vk::CommandPoolCreateInfo createInfo;
    uint32_t queueFamilyIndex = queueFamilyIndices.graphicsQueue.value();
    createInfo.setQueueFamilyIndex(queueFamilyIndex)
        .setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer | ::vk::CommandPoolCreateFlagBits::eTransient);
    cmdPool_ = device_.createCommandPool(createInfo);
}

void Device::executeCmd(const RecordCmdFunc& func) {
    ::vk::CommandBufferAllocateInfo allocInfo(cmdPool_, ::vk::CommandBufferLevel::ePrimary, 1);
    auto commandBuffer = device_.allocateCommandBuffers(allocInfo)[0];
    ::vk::CommandBufferBeginInfo beginInfo(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer.begin(beginInfo);
    if (func) {
        func(commandBuffer);
    }
    commandBuffer.end();
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(commandBuffer);
    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
    device_.waitIdle();
    device_.freeCommandBuffers(cmdPool_, commandBuffer);
}

auto Device::getVKInstance() -> ::vk::Instance& { return vkInstance; }

auto Device::getSurface() -> ::vk::SurfaceKHR& { return vkSurfaceKHR; }

auto Device::getPhysicalDevice() -> ::vk::PhysicalDevice& { return phyDevice; }

auto Device::getGraphicsQueue() -> ::vk::Queue& { return graphicsQueue; }

auto Device::getPresentQueue() -> ::vk::Queue& { return presentQueue; }

auto Device::findSupportedFormat(const std::vector<::vk::Format>& candidates, ::vk::ImageTiling tiling,
                                 ::vk::FormatFeatureFlags features) -> ::vk::Format {
    for (::vk::Format format : candidates) {
        ::vk::FormatProperties props = phyDevice.getFormatProperties(format);

        if (tiling == ::vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == ::vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

auto Device::logicalDevice() -> ::vk::Device& { return device_; }

auto Device::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) -> uint32_t {
    ::vk::PhysicalDeviceMemoryProperties memProperties = phyDevice.getMemoryProperties();
    ;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Device::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, ::vk::Format format,
                         ::vk::SampleCountFlagBits numSamples, ::vk::ImageTiling tiling, ::vk::ImageUsageFlags usage,
                         ::vk::MemoryPropertyFlags properties, ::vk::Image& image, ::vk::DeviceMemory& imageMemory) {
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

auto Device::createImageView(::vk::Image image, ::vk::Format format, ::vk::ImageAspectFlags aspectFlags,
                             uint32_t mipLevels) -> ::vk::ImageView {
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

auto Device::getMaxAnisotropy() -> float {
    ::vk::PhysicalDeviceProperties properties = phyDevice.getProperties();
    return properties.limits.maxSamplerAnisotropy;
}

auto Device::getMaxUsableSampleCount() -> ::vk::SampleCountFlagBits {
    vk::PhysicalDeviceProperties physicalDevice = phyDevice.getProperties();
    ::vk::SampleCountFlags counts =
        physicalDevice.limits.framebufferColorSampleCounts & physicalDevice.limits.framebufferDepthSampleCounts;
    if (counts & ::vk::SampleCountFlagBits::e64) {
        return ::vk::SampleCountFlagBits::e64;
    }
    if (counts & ::vk::SampleCountFlagBits::e32) {
        return ::vk::SampleCountFlagBits::e32;
    }
    if (counts & ::vk::SampleCountFlagBits::e16) {
        return ::vk::SampleCountFlagBits::e16;
    }
    if (counts & ::vk::SampleCountFlagBits::e8) {
        return ::vk::SampleCountFlagBits::e8;
    }
    if (counts & ::vk::SampleCountFlagBits::e4) {
        return ::vk::SampleCountFlagBits::e4;
    }
    if (counts & ::vk::SampleCountFlagBits::e2) {
        return ::vk::SampleCountFlagBits::e2;
    }

    return ::vk::SampleCountFlagBits::e1;
}

auto Device::checkDeviceExtensionSupport(::vk::PhysicalDevice& checkDevice) -> bool {
    std::vector<vk::ExtensionProperties> availableExtensions = checkDevice.enumerateDeviceExtensionProperties();
    ::std::set<::std::string> requireExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requireExtensions.erase(extension.extensionName);
    }

    return requireExtensions.empty();
}

auto Device::isDeviceSuitable(::vk::PhysicalDevice& checkDevice) -> bool {
    bool extensionsSupported = checkDeviceExtensionSupport(checkDevice);
    bool swapChainAdeqate = false;
    if (extensionsSupported) {
        SwapchainSupportDetails swapchainSupport = querySwapchainSupport(checkDevice);
        swapChainAdeqate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }
    ::vk::PhysicalDeviceFeatures deviceFeatures = checkDevice.getFeatures();
    auto indices = queryQueueFamilyIndices(checkDevice);
    return extensionsSupported && swapChainAdeqate && deviceFeatures.samplerAnisotropy && indices.isComplete();
}

auto Device::querySwapchainSupport(::vk::PhysicalDevice device) -> SwapchainSupportDetails {
    SwapchainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(vkSurfaceKHR);
    details.formats = device.getSurfaceFormatsKHR(vkSurfaceKHR);
    details.presentModes = device.getSurfacePresentModesKHR(vkSurfaceKHR);
    return details;
}

}  // namespace core
