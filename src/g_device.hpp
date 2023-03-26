#ifndef G_DEVICE_HPP
#define G_DEVICE_HPP
#include <vulkan/vulkan.hpp>
#include <memory>
#include <cstdint>
#include <optional>
#include <functional>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include<glm/gtc/constants.hpp>

namespace g{
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
        ::vk::Device device;
        ::vk::Queue graphicsQueue;
        ::vk::Queue presentQueue;
        ::vk::SurfaceKHR vkSurfaceKHR;
        ::vk::Instance vkInstance;
        void pickupPhyiscalDevice();
        void createDevice();
        void queryQueueFamilyIndices();
        void getQueues();
        void createInstance(const std::vector<const char*>& instanceExtends);
        Device(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc);
        static ::std::unique_ptr<Device> instance;
    public:

        struct QueueFamilyIndices final{
            ::std::optional<uint32_t> graphicsQueue;
            ::std::optional<uint32_t> queueCount;
            ::std::optional<uint32_t> presentQueue;
            bool isComplete(){return graphicsQueue.has_value() && presentQueue.has_value();}
        };

        static void init(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc);
        static Device& getInstance(){return *instance;}
        static void quit();
        QueueFamilyIndices queueFamilyIndices;
        uint32_t findMemoryType(uint32_t typeFilter, ::vk::MemoryPropertyFlags properties);
        void createBuffer(::vk::DeviceSize size, ::vk::BufferUsageFlags usgae, ::vk::MemoryPropertyFlags properties, 
            ::vk::Buffer& buffer, ::vk::DeviceMemory& bufferMemory);
        ::vk::Instance& getVKInstance();
        ::vk::SurfaceKHR& getSurface();
        ::vk::PhysicalDevice& getPhysicalDevice();
        ::vk::Queue& getGraphicsQueue();
        ::vk::Queue& getPresentQueue();
        ::vk::Device& getVKDevice();
        ::vk::Format findSupportedFormat(const std::vector<::vk::Format> &candidates, ::vk::ImageTiling tiling, ::vk::FormatFeatureFlags features);
        ~Device();

        
    };
}



#endif