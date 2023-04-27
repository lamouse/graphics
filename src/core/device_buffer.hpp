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
        static auto create(Device& device, ::vk::BufferUsageFlags usage, const void* data, ::vk::DeviceSize size) -> DeviceBuffer;
        auto operator()() -> ::vk::Buffer{return buffer_;}
        ~DeviceBuffer();

        DeviceBuffer(const DeviceBuffer&) = delete;
        DeviceBuffer(DeviceBuffer&&) = default;
        auto operator=(const DeviceBuffer&) -> DeviceBuffer = delete;
        auto operator=(DeviceBuffer&&) -> DeviceBuffer& = default;
    };
}