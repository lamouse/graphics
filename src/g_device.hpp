#ifndef G_DEVICE_HPP
#define G_DEVICE_HPP
#include <vulkan/vulkan.hpp>
#include <memory>
#include <cstdint>
#include <optional>

namespace g{
    using CreateSurfaceFunc = ::std::function<vk::SurfaceKHR(vk::Instance)>;

    class Device final
    {
    private:
        ::vk::PhysicalDevice phyDevice;
        ::vk::Device device;
        ::vk::Queue graphicsQueue;

        ::vk::SurfaceKHR vkSurfaceKHR;
        ::vk::Instance vkInstance;
        void pickupPhyiscalDevice();
        void createDevice();
        void queryQueueFamilyIndices();
        void getQueues();
        Device(const ::vk::Instance& instance, ::vk::SurfaceKHR vkSurfaceKHR);
        static ::std::unique_ptr<Device> instance;
    public:

        struct QueueFamilyIndices final{
            ::std::optional<uint32_t> graphicsQueue;
            ::std::optional<uint32_t> queueCount;
            ::std::optional<uint32_t> presentQueue;
            bool isComplete(){return graphicsQueue.has_value() && presentQueue.has_value();}
        };

        static void init(const ::vk::Instance& vkInstance, ::vk::SurfaceKHR vkSurfaceKHR);
        static Device& getInstance(){return *instance;}
        static void quit();
        QueueFamilyIndices queueFamilyIndices;
        
        ::vk::Instance& getVKInstance();
        ::vk::SurfaceKHR& getSurface();
        ::vk::PhysicalDevice& getPhysicalDevice();
        ::vk::Queue& getGraphicsQueue();
        ::vk::Device& getVKDevice();
        ~Device();

        
    };
    

}

#endif