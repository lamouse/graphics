#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace core {

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
using CreateSurfaceFunc = ::std::function<::vk::SurfaceKHR(::vk::Instance)>;

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
        ::vk::DebugUtilsMessengerEXT debugMessenger_;
        bool enableValidationLayers_;
        void pickupPhysicalDevice(const ::std::vector<const char*>& deviceExtensions);
        void createLogicalDevice(const ::std::vector<const char*>& deviceExtensions);
        void getQueues();
        void initCmdPool();
        void createInstance(const std::vector<const char*>& instanceExtends);
        auto isDeviceSuitable(::vk::PhysicalDevice& checkDevice, const ::std::vector<const char*>& deviceExtensions) -> bool;
        auto getMaxUsableSampleCount() -> ::vk::SampleCountFlagBits;
        /**
         * @brief Validation layers
         *
         */
        ::std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    public:
        struct QueueFamilyIndices {
                uint32_t graphicsQueue;
                uint32_t computeQueue;
                uint32_t presentQueue;
        };
        Device(Device&) = delete;
        auto operator=(const Device&) -> Device& = delete;
        Device(Device&&) = delete;
        auto operator=(Device&&) -> Device& = delete;

        Device(const std::vector<const char*>& instanceExtends, const ::std::vector<const char*>& deviceExtensions, const CreateSurfaceFunc& createFunc,
               bool enableValidationLayers = false);
        auto queryQueueFamilyIndices(::vk::PhysicalDevice device) -> ::std::optional<QueueFamilyIndices>;
        using RecordCmdFunc = std::function<void(vk::CommandBuffer&)>;
        QueueFamilyIndices queueFamilyIndices;
        auto getMaxMsaaSamples() -> ::vk::SampleCountFlagBits { return getMaxUsableSampleCount(); }
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
        void executeCmd(const RecordCmdFunc& func) const;
        auto getMaxAnisotropy() -> float;
        ~Device();
};
}  // namespace core
