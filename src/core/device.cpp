#include "device.hpp"

#include <spdlog/spdlog.h>
#include <ranges>
#include <set>
#include <mutex>
#include <fmt/core.h>

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
        default:
            spdlog::error("validation layer unknow messageSeverity: {}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

inline void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.setMessageSeverity(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    createInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                              vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                              vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
    createInfo.pfnUserCallback = debugCallback;
}

inline auto setupDebugMessenger(::vk::Instance& instance) {
    ::vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    return *instance.createDebugUtilsMessengerEXTUnique(createInfo, nullptr,
                                                        vk::DispatchLoaderDynamic{instance, vkGetInstanceProcAddr});
}

auto checkValidationLayerSupport(const auto& validationLayers) -> bool {
    std::vector<::vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char* layerName : validationLayers) {
        auto layerProperty = ::std::ranges::find_if(availableLayers, [&layerName](const auto& layerProperties) {
            return strcmp(layerName, layerProperties.layerName) == 0;
        });
        if (layerProperty == availableLayers.end()) {
            return false;
        }
    }

    return true;
}

auto checkDeviceExtensionSupport(const ::vk::PhysicalDevice& checkDevice, const ::std::vector<const char*>& extensions)
    -> bool {
    auto availableExtensions = checkDevice.enumerateDeviceExtensionProperties();
    ::std::set<::std::string> requireExtensions(extensions.begin(), extensions.end());

    for (const auto& extension : availableExtensions) {
        requireExtensions.erase(extension.extensionName);
    }

    return requireExtensions.empty();
}

/**
 * @brief Validation layers
 *
 */
const ::std::array<const char*, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};
bool device_init_finish = false;
class VKResource {
    public:
        ::vk::PhysicalDevice phyDevice;
        ::vk::Device device_;
        ::vk::Queue graphicsQueue;
        ::vk::Queue presentQueue;
        ::vk::Queue computeQueue;
        ::vk::SurfaceKHR vkSurfaceKHR;
        ::vk::Instance vkInstance;
        ::vk::CommandPool cmdPool_;
        ~VKResource() {
            if (device_init_finish) {
                device_.destroyCommandPool(cmdPool_);
                device_.destroy();
                vkInstance.destroySurfaceKHR(vkSurfaceKHR);
                vkInstance.destroy();
            }
        }
};
VKResource resource;
Device::QueueFamilyIndices queueFamilyIndices;

bool enableValidationLayers_;
::std::once_flag device_init_once_;

void pickupPhysicalDevice(const ::std::vector<const char*>& deviceExtensions);
void createLogicalDevice(const ::std::vector<const char*>& deviceExtensions);
void getQueues();
void initCmdPool();
void createInstance(const std::vector<const char*>& instanceExtends);
auto isDeviceSuitable(::vk::PhysicalDevice& checkDevice, const ::std::vector<const char*>& deviceExtensions) -> bool;
auto getMaxUsableSampleCount() -> ::vk::SampleCountFlagBits;
auto querySwapchainSupport(::vk::PhysicalDevice device) -> SwapchainSupportDetails;
void queryQueueFamilyIndices(::vk::PhysicalDevice device);


void createInstance(const std::vector<const char*>& instanceExtends) {
    if (enableValidationLayers_ && !checkValidationLayerSupport(validationLayers)) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    uint32_t instanceVersion = ::vk::enumerateInstanceVersion();
    spdlog::debug("vk instance version, Major: {}, Minor: {}, Patch: {}", VK_API_VERSION_MAJOR(instanceVersion),
                  VK_API_VERSION_MINOR(instanceVersion), VK_API_VERSION_PATCH(instanceVersion));
    uint32_t version = VK_MAKE_VERSION(1, 3, 0);
    ::vk::ApplicationInfo appInfo{"graphics", version, "engine", version, version};

    ::vk::InstanceCreateInfo createInfo;

    if (enableValidationLayers_) {
        createInfo.setPEnabledLayerNames(validationLayers);
        ::vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }

    createInfo
        .setPApplicationInfo(&appInfo)
#if defined(VK_USE_PLATFORM_MACOS_MVK)
        .setFlags(::vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR)
#endif
        .setPEnabledExtensionNames(instanceExtends);
    resource.vkInstance = vk::createInstance(createInfo);
    if (enableValidationLayers_) {
        setupDebugMessenger(resource.vkInstance);
    }
}

void pickupPhysicalDevice(const ::std::vector<const char*>& deviceExtensions) {
    auto phyDevices = resource.vkInstance.enumeratePhysicalDevices();
    if (phyDevices.empty()) {
        throw ::std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    const auto findPhyDevice =
        ::std::ranges::find_if(phyDevices, [&](auto& device) { return isDeviceSuitable(device, deviceExtensions); });

    if (findPhyDevice != phyDevices.end()) {
        resource.phyDevice = *findPhyDevice;
        queryQueueFamilyIndices(resource.phyDevice);
    } else {
        throw ::std::runtime_error("failed to find a suitable GPU!");
    }
}

void createLogicalDevice(const ::std::vector<const char*>& deviceExtensions) {
    ::std::vector<::vk::DeviceQueueCreateInfo> queueInfos;
    const ::std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilyIndices.graphicsIndex(), queueFamilyIndices.presentIndex(), queueFamilyIndices.computeIndex()};
    constexpr float priorities = 1.f;

    for (const auto queueFamily : uniqueQueueFamilies) {
        ::vk::DeviceQueueCreateInfo queueInfo(::vk::DeviceQueueCreateFlags(), queueFamily, 1, &priorities);
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
    resource.device_ = resource.phyDevice.createDevice(createInfo);
}

void queryQueueFamilyIndices(::vk::PhysicalDevice device) {
    auto properties = device.getQueueFamilyProperties();
    int index = 0;
    for (const auto& property : properties) {
        if ((property.queueFlags & ::vk::QueueFlagBits::eGraphics)) {
            queueFamilyIndices.graphicsQueue = index;
        }
        if ((property.queueFlags & ::vk::QueueFlagBits::eCompute)) {
            queueFamilyIndices.computeQueue = index;
        }

        if (device.getSurfaceSupportKHR(index, resource.vkSurfaceKHR)) {
            queueFamilyIndices.presentQueue = index;
        }
        if (queueFamilyIndices.isComplete()) {
            break;
        }
        index++;
    }
}


void getQueues() {
    resource.graphicsQueue = resource.device_.getQueue(queueFamilyIndices.graphicsIndex(), 0);
    resource.presentQueue = resource.device_.getQueue(queueFamilyIndices.presentIndex(), 0);
    resource.computeQueue = resource.device_.getQueue(queueFamilyIndices.computeIndex(), 0);
}

void initCmdPool() {
    ::vk::CommandPoolCreateInfo createInfo;
    createInfo.setQueueFamilyIndex(queueFamilyIndices.graphicsIndex())
        .setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer | ::vk::CommandPoolCreateFlagBits::eTransient);
    resource.cmdPool_ = resource.device_.createCommandPool(createInfo);
}

auto getMaxUsableSampleCount() -> ::vk::SampleCountFlagBits {
    vk::PhysicalDeviceProperties physicalDevice = resource.phyDevice.getProperties();
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

auto isDeviceSuitable(::vk::PhysicalDevice& checkDevice, const ::std::vector<const char*>& deviceExtensions) -> bool {
    bool extensionsSupported = checkDeviceExtensionSupport(checkDevice, deviceExtensions);
    bool swapChainAdeqate = false;
    if (extensionsSupported) {
        SwapchainSupportDetails swapchainSupport = querySwapchainSupport(checkDevice);
        swapChainAdeqate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }
    ::vk::PhysicalDeviceFeatures deviceFeatures = checkDevice.getFeatures();
    queryQueueFamilyIndices(checkDevice);
    return extensionsSupported && swapChainAdeqate && deviceFeatures.samplerAnisotropy &&
           queueFamilyIndices.isComplete();
}

auto querySwapchainSupport(::vk::PhysicalDevice device) -> SwapchainSupportDetails {
    SwapchainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(resource.vkSurfaceKHR);
    details.formats = device.getSurfaceFormatsKHR(resource.vkSurfaceKHR);
    details.presentModes = device.getSurfacePresentModesKHR(resource.vkSurfaceKHR);
    return details;
}

}  // namespace

Device::Device(const std::source_location& location) {
    std::call_once(device_init_once_, [&]() {
        throw std::runtime_error(fmt::format("use core device fist need call Device::init()\n file {}:{}", location.file_name(), location.line()));
    });

}

 void Device::init(const std::vector<const char*>& instanceExtends,
                       const ::std::vector<const char*>& deviceExtensions,
                 const CreateSurfaceFunc& createFunc, bool enableValidationLayers) {
    std::call_once(device_init_once_, [&]() {
        enableValidationLayers_ = enableValidationLayers;
        createInstance(instanceExtends);
        resource.vkSurfaceKHR = createFunc(resource.vkInstance);

        pickupPhysicalDevice(deviceExtensions);
        createLogicalDevice(deviceExtensions);
        getQueues();
        initCmdPool();
        device_init_finish = true;
    });
}

Device::~Device() {

}

auto Device::getMaxMsaaSamples() -> ::vk::SampleCountFlagBits { return getMaxUsableSampleCount(); }

void Device::createBuffer(::vk::DeviceSize size, ::vk::BufferUsageFlags usage, ::vk::MemoryPropertyFlags properties,
                          ::vk::Buffer& buffer, ::vk::DeviceMemory& bufferMemory) {
    ::vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(size).setUsage(usage).setSharingMode(::vk::SharingMode::eExclusive);
    buffer = resource.device_.createBuffer(bufferInfo);
    const ::vk::MemoryRequirements memoryRequirements = resource.device_.getBufferMemoryRequirements(buffer);
    ::vk::MemoryAllocateInfo allocateInfo;
    allocateInfo.setAllocationSize(memoryRequirements.size)
        .setMemoryTypeIndex(findMemoryType(memoryRequirements.memoryTypeBits, properties));
    bufferMemory = resource.device_.allocateMemory(allocateInfo);
    resource.device_.bindBufferMemory(buffer, bufferMemory, 0);
}



void Device::executeCmd(const CmdFunc& func) const {
    const ::vk::CommandBufferAllocateInfo allocInfo(resource.cmdPool_, ::vk::CommandBufferLevel::ePrimary, 1);
    auto commandBuffer = resource.device_.allocateCommandBuffers(allocInfo)[0];
    const ::vk::CommandBufferBeginInfo beginInfo(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer.begin(beginInfo);
    if (func) {
        func(commandBuffer);
    }
    commandBuffer.end();
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(commandBuffer);
    resource.graphicsQueue.submit(submitInfo);
    resource.graphicsQueue.waitIdle();
    resource.device_.waitIdle();
    resource.device_.freeCommandBuffers(resource.cmdPool_, commandBuffer);
}

auto Device::getVKInstance() -> ::vk::Instance& { return resource.vkInstance; }

auto Device::getSurface() -> ::vk::SurfaceKHR& { return resource.vkSurfaceKHR; }

auto Device::getPhysicalDevice() -> ::vk::PhysicalDevice& { return resource.phyDevice; }

auto Device::findSupportedFormat(const std::vector<::vk::Format>& candidates, ::vk::ImageTiling tiling,
                                 ::vk::FormatFeatureFlags features) -> ::vk::Format {
    for (const auto& format : candidates) {
        const auto formatProperties = resource.phyDevice.getFormatProperties(format);

        if (tiling == ::vk::ImageTiling::eLinear && (formatProperties.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == ::vk::ImageTiling::eOptimal && (formatProperties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

auto Device::logicalDevice() -> ::vk::Device& { return resource.device_; }
auto Device::getCommandPool() -> ::vk::CommandPool& { return resource.cmdPool_; }

auto Device::getQueue(DeviceQueue queue) -> ::vk::Queue& {
    switch (queue) {
        case DeviceQueue::compute:
            return resource.computeQueue;
        case DeviceQueue::graphics:
            return resource.graphicsQueue;
        case DeviceQueue::present:
            return resource.presentQueue;
//        default:
//                   throw std::runtime_error("un know logic device queue type");
    }
}

auto Device::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) -> uint32_t {
    ::vk::PhysicalDeviceMemoryProperties memProperties = resource.phyDevice.getMemoryProperties();
    ;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & ((uint32_t)1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
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

    image = resource.device_.createImage(imageInfo);
    ::vk::MemoryRequirements memRequirements = resource.device_.getImageMemoryRequirements(image);
    ::vk::MemoryAllocateInfo allocInfo;
    allocInfo.setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(findMemoryType(memRequirements.memoryTypeBits, properties));
    imageMemory = resource.device_.allocateMemory(allocInfo);
    resource.device_.bindImageMemory(image, imageMemory, 0);
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

    return resource.device_.createImageView(viewInfo);
}

auto Device::getMaxAnisotropy() -> float {
    ::vk::PhysicalDeviceProperties properties = resource.phyDevice.getProperties();
    return properties.limits.maxSamplerAnisotropy;
}

auto Device::getQueueFamilyIndices() -> QueueFamilyIndices { return queueFamilyIndices; }

auto Device::querySwapchainSupport() -> SwapchainSupportDetails { return core::querySwapchainSupport(resource.phyDevice); }
}  // namespace core
