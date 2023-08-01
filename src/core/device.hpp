#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace core {
/**
 * @brief Validation layers
 *
 */
const ::std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

/**
 * @brief Device extensions
 *
 */
const ::std::vector<const char*> deviceExtensions = {
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    "VK_KHR_portability_subset",  // "VK_KHR_portability_subset" macos
#endif
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

/**
 * @brief Swapchain support
 *
 */
struct SwapchainSupportDetails {
        ::vk::SurfaceCapabilitiesKHR capabilities;
        ::std::vector<::vk::SurfaceFormatKHR> formats;
        ::std::vector<::vk::PresentModeKHR> presentModes;
};

/**
 * @brief create device get surface
 *
 */
using CreateSurfaceFunc = ::std::function<vk::SurfaceKHR(vk::Instance)>;

/**
 * @brief vulkan device info add util
 *
 */
class Device final {
    private:
        ::vk::PhysicalDevice phyDevice;
        ::vk::Device device_;
        ::vk::Queue graphicsQueue;
        ::vk::Queue presentQueue;
        ::vk::Queue computeQueue;
        ::vk::SurfaceKHR vkSurfaceKHR;
        ::vk::Instance vkInstance;
        ::vk::CommandPool cmdPool_;
        ::vk::SampleCountFlagBits maxMsaaSamples_;
        ::vk::DebugUtilsMessengerEXT debugMessenger_;
        bool enableValidationLayers_;
        void pickupPhysicalDevice();
        void createLogicalDevice();
        void getQueues();
        void initCmdPool();
        void createInstance(const std::vector<const char*>& instanceExtends);
        static auto checkDeviceExtensionSupport(::vk::PhysicalDevice& checkDevice) -> bool;
        auto isDeviceSuitable(::vk::PhysicalDevice& checkDevice) -> bool;
        auto getMaxUsableSampleCount() -> ::vk::SampleCountFlagBits;

    public:
        struct QueueFamilyIndices final {
                ::std::optional<uint32_t> graphicsQueue;
                ::std::optional<uint32_t> computeQueue;
                ::std::optional<uint32_t> presentQueue;
                [[nodiscard]] auto isComplete() const -> bool {
                    return graphicsQueue.has_value() && presentQueue.has_value();
                }
        };
        Device(const std::vector<const char*>& instanceExtends, const CreateSurfaceFunc& createFunc,
               bool enableValidationLayers = false);
        auto queryQueueFamilyIndices(::vk::PhysicalDevice device) -> QueueFamilyIndices;
        using RecordCmdFunc = std::function<void(vk::CommandBuffer&)>;
        QueueFamilyIndices queueFamilyIndices;
        auto getMaxMsaaSamples() -> ::vk::SampleCountFlagBits { return maxMsaaSamples_; }
        auto findMemoryType(uint32_t typeFilter, ::vk::MemoryPropertyFlags properties) -> uint32_t;
        void createBuffer(::vk::DeviceSize size, ::vk::BufferUsageFlags usage, ::vk::MemoryPropertyFlags properties,
                          ::vk::Buffer& buffer, ::vk::DeviceMemory& bufferMemory);
        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, ::vk::Format format,
                         ::vk::SampleCountFlagBits numSamples, ::vk::ImageTiling tiling, ::vk::ImageUsageFlags usage,
                         ::vk::MemoryPropertyFlags properties, ::vk::Image& image, ::vk::DeviceMemory& imageMemory);
        auto createImageView(::vk::Image image, ::vk::Format format, ::vk::ImageAspectFlags aspectFlags,
                             uint32_t mipLevels) -> ::vk::ImageView;
        auto getVKInstance() -> ::vk::Instance&;
        auto getSurface() -> ::vk::SurfaceKHR&;
        auto getPhysicalDevice() -> ::vk::PhysicalDevice&;
        auto getGraphicsQueue() -> ::vk::Queue&;
        auto getPresentQueue() -> ::vk::Queue&;
        auto getComputeQueue() -> ::vk::Queue& { return computeQueue; }
        /**
         * @brief vk logical device
         *
         * @return ::vk::Device&
         */
        auto logicalDevice() -> ::vk::Device&;
        auto findSupportedFormat(const std::vector<::vk::Format>& candidates, ::vk::ImageTiling tiling,
                                 ::vk::FormatFeatureFlags features) -> ::vk::Format;
        auto getCommandPool() -> ::vk::CommandPool { return cmdPool_; }
        auto querySwapchainSupport(::vk::PhysicalDevice device) -> SwapchainSupportDetails;
        void executeCmd(const RecordCmdFunc& func);
        auto getMaxAnisotropy() -> float;
        ~Device();
};
}  // namespace core
