#ifndef G_DEVICE_HPP
#define G_DEVICE_HPP
#include <stdint.h>
#include <vulkan/vulkan.hpp>
#include <memory>
#include <cstdint>
#include <optional>
#include <functional>

namespace core{

    struct SwapchainSupportDetails
    {
        ::vk::SurfaceCapabilitiesKHR capabilities;
        ::std::vector<::vk::SurfaceFormatKHR> formats;
        ::std::vector<::vk::PresentModeKHR> presentModes;
    };

    using CreateSurfaceFunc = ::std::function<vk::SurfaceKHR(vk::Instance)>;

    class Device final
    {
    private:
        ::vk::PhysicalDevice phyDevice;
        ::vk::Device device_;
        ::vk::Queue graphicsQueue;
        ::vk::Queue presentQueue;
        ::vk::SurfaceKHR vkSurfaceKHR;
        ::vk::Instance vkInstance;
        ::vk::CommandPool cmdPool_;
        ::vk::SampleCountFlags msaaSamples;
        void pickupPhyiscalDevice();
        void createDevice();
        void getQueues();
        void initCmdPool();
        void createInstance(const std::vector<const char*>& instanceExtends);
        bool checkDeviceExtensionSupport(::vk::PhysicalDevice& checkDevice);
        bool isDeviceSuitable(::vk::PhysicalDevice& checkDevice);
        static ::std::unique_ptr<Device> instance;
        
        const ::std::vector<const char*> deviceExtensions = {
#if defined(VK_USE_PLATFORM_MACOS_MVK)      
                "VK_KHR_portability_subset",    // "VK_KHR_portability_subset" macos     
#endif 
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    public:
        struct QueueFamilyIndices final{
            ::std::optional<uint32_t> graphicsQueue;
            ::std::optional<uint32_t> presentQueue;
            bool isComplete(){return graphicsQueue.has_value() && presentQueue.has_value();}
        };
        Device(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc);
        ::vk::SampleCountFlagBits getMaxUsableSampleCount();
        QueueFamilyIndices queryQueueFamilyIndices(::vk::PhysicalDevice device);
        using RecordCmdFunc = std::function<void(vk::CommandBuffer&)>;
        QueueFamilyIndices queueFamilyIndices;
        uint32_t findMemoryType(uint32_t typeFilter, ::vk::MemoryPropertyFlags properties);
        void createBuffer(::vk::DeviceSize size, ::vk::BufferUsageFlags usgae, ::vk::MemoryPropertyFlags properties, 
            ::vk::Buffer& buffer, ::vk::DeviceMemory& bufferMemory);
        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels,::vk::Format format, ::vk::SampleCountFlagBits numSamples ,::vk::ImageTiling tiling, 
                    ::vk::ImageUsageFlags usage, ::vk::MemoryPropertyFlags properties, ::vk::Image& image, ::vk::DeviceMemory& imageMemory);
        ::vk::ImageView createImageView(::vk::Image image, ::vk::Format format, ::vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
        ::vk::Instance& getVKInstance();
        ::vk::SurfaceKHR& getSurface();
        ::vk::PhysicalDevice& getPhysicalDevice();
        ::vk::Queue& getGraphicsQueue();
        ::vk::Queue& getPresentQueue();
        ::vk::Device& getVKDevice();
        ::vk::Format findSupportedFormat(const std::vector<::vk::Format> &candidates, ::vk::ImageTiling tiling, ::vk::FormatFeatureFlags features);
        ::vk::CommandPool getCommandPool(){return cmdPool_;}
        SwapchainSupportDetails querySwapchainSupport(::vk::PhysicalDevice device);
        void excuteCmd(RecordCmdFunc func);
        float getMaxAnisotropy();
        ~Device();
        
    };
}



#endif