#include "device_buffer.hpp"
#include "device.hpp"
#include "staging_buffer.hpp"


namespace core {

void copyBuffer (::vk::Buffer srcBuffer, vk::Buffer dstBuffer, ::vk::DeviceSize size);

DeviceBuffer::DeviceBuffer(::vk::Buffer& buffer, ::vk::DeviceMemory& memory)
    : buffer_(buffer), bufferMemory_(memory){}

auto DeviceBuffer::create(::vk::BufferUsageFlags usage, const void* data, ::vk::DeviceSize size)
    -> DeviceBuffer {
    Device device;
    core::StagingBuffer stagingBuffer(device, size, data);
    ::vk::Buffer vertexBuffer;
    ::vk::DeviceMemory vertexBufferMemory;

    device.createBuffer(size, usage | ::vk::BufferUsageFlagBits::eTransferDst,
                        ::vk::MemoryPropertyFlagBits::eDeviceLocal | ::vk::MemoryPropertyFlagBits::eHostCoherent,
                        vertexBuffer, vertexBufferMemory);
    copyBuffer(stagingBuffer.getBuffer(), vertexBuffer, size);
    return {vertexBuffer, vertexBufferMemory};
}

DeviceBuffer::~DeviceBuffer() {
    Device device;
    device.logicalDevice().destroyBuffer(buffer_);
    device.logicalDevice().freeMemory(bufferMemory_);
}

void copyBuffer(::vk::Buffer srcBuffer, vk::Buffer dstBuffer, ::vk::DeviceSize size) {
    Device device;
    device.executeCmd([&](auto& cmdBuf) {
        ::vk::BufferCopy copyRegion{};
        copyRegion.size = size;
        cmdBuf.copyBuffer(srcBuffer, dstBuffer, copyRegion);
    });
}

}  // namespace core