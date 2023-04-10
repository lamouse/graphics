#include "device_buffer.hpp"
#include "staging_buffer.hpp"

namespace core {

void copyBuffer(Device& device, ::vk::Buffer srcBuffer, vk::Buffer dstBuffer, ::vk::DeviceSize size);

DeviceBuffer::DeviceBuffer(::vk::Buffer& buffer, ::vk::DeviceMemory& memory, ::vk::Device& device):buffer_(buffer), bufferMemory_(memory),device_(device)
{

}

 DeviceBuffer DeviceBuffer::create(Device& device, ::vk::BufferUsageFlags usage, const void* data, ::vk::DeviceSize size)
 {
    core::StagingBuffer stagingBuffer(device, size, data);
    ::vk::Buffer vertexBuffer;
    ::vk::DeviceMemory vertexBufferMemory;

    device.createBuffer(size, usage | ::vk::BufferUsageFlagBits::eTransferDst, 
                ::vk::MemoryPropertyFlagBits::eDeviceLocal|::vk::MemoryPropertyFlagBits::eHostCoherent, vertexBuffer,vertexBufferMemory);
    copyBuffer(device, stagingBuffer.getBuffer(), vertexBuffer, size);
    return DeviceBuffer(vertexBuffer, vertexBufferMemory, device.getVKDevice());
 }



DeviceBuffer::~DeviceBuffer()
{
    device_.get().destroyBuffer(buffer_);
    device_.get().freeMemory(bufferMemory_);
}

void copyBuffer(Device& device, ::vk::Buffer srcBuffer, vk::Buffer dstBuffer, ::vk::DeviceSize size)
{
    device.excuteCmd([&](auto& cmdBuf){
        ::vk::BufferCopy copyRegion{};
        copyRegion.size = size;
        cmdBuf.copyBuffer(srcBuffer, dstBuffer, copyRegion);
    });
}

}