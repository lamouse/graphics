#ifndef G_DEVICE_HPP
#define G_DEVICE_HPP
#include <stdint.h>
#include <vulkan/vulkan.hpp>
#include <memory>
#include <cstdint>
#include <optional>
#include <functional>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace g{

    struct SwapchainSupportDetails
    {
        ::vk::SurfaceCapabilitiesKHR capabilities;
        ::std::vector<::vk::SurfaceFormatKHR> formats;
        ::std::vector<::vk::PresentModeKHR> presentModes;
    };

    struct SimplePushConstantData
    {
        ::glm::mat4 transform{1.f};
        ::glm::vec3 color;
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
        ::vk::SampleCountFlags getMaxUsableSampleCount();
        Device(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc);
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

        QueueFamilyIndices queryQueueFamilyIndices(::vk::PhysicalDevice device);
        using RecordCmdFunc = std::function<void(vk::CommandBuffer&)>;
        static void init(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc);
        static Device& getInstance(){return *instance;}
        static void quit();
        QueueFamilyIndices queueFamilyIndices;
        uint32_t findMemoryType(uint32_t typeFilter, ::vk::MemoryPropertyFlags properties);
        void createBuffer(::vk::DeviceSize size, ::vk::BufferUsageFlags usgae, ::vk::MemoryPropertyFlags properties, 
            ::vk::Buffer& buffer, ::vk::DeviceMemory& bufferMemory);
        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels,::vk::Format format, ::vk::ImageTiling tiling, ::vk::ImageUsageFlags usage, 
                                    ::vk::MemoryPropertyFlags properties, ::vk::Image& image, ::vk::DeviceMemory& imageMemory);
        ::vk::ImageView createImageView(::vk::Image image, ::vk::Format format, uint32_t mipLevels);
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