#pragma once
#include "device.hpp"
#include <vulkan/vulkan.hpp>
#include <functional>

namespace core {
    class DeviceBuffer{
    private:
        ::vk::Buffer buffer_;
        ::vk::DeviceMemory bufferMemory_;
        ::std::reference_wrapper<::vk::Device> device_;
        DeviceBuffer(::vk::Buffer& buffer, ::vk::DeviceMemory& memory, ::vk::Device& device);
    public:
        static DeviceBuffer create(Device& device, ::vk::BufferUsageFlags usage, const void* data, ::vk::DeviceSize size);
        ::vk::Buffer operator()(){return buffer_;}
        ~DeviceBuffer();

        DeviceBuffer(const DeviceBuffer&) = delete;
        DeviceBuffer(DeviceBuffer&&) = default;
        DeviceBuffer operator=(const DeviceBuffer&) = delete;
        DeviceBuffer& operator=(DeviceBuffer&&) = default;
    };
}