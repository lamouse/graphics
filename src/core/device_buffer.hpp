#pragma once
#include <functional>
#include <vulkan/vulkan.hpp>


namespace core {
class DeviceBuffer {
    private:
        ::vk::Buffer buffer_;
        ::vk::DeviceMemory bufferMemory_;
        DeviceBuffer(::vk::Buffer& buffer, ::vk::DeviceMemory& memory);

    public:
        static auto create(::vk::BufferUsageFlags usage, const void* data, ::vk::DeviceSize size)
            -> DeviceBuffer;
        auto operator()()const -> ::vk::Buffer { return buffer_; }
        ~DeviceBuffer();

        DeviceBuffer(const DeviceBuffer&) = delete;
        DeviceBuffer(DeviceBuffer&&) = default;
        auto operator=(const DeviceBuffer&) -> DeviceBuffer = delete;
        auto operator=(DeviceBuffer&&) -> DeviceBuffer& = default;
};
}  // namespace core