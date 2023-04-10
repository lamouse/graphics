#include "staging_buffer.hpp"
#include <cstdlib>

namespace core {

StagingBuffer::StagingBuffer(Device& device, ::vk::DeviceSize size, const void* bufferData):device_(device.getVKDevice()),buff_size(size)
{
    device.createBuffer(size, ::vk::BufferUsageFlagBits::eTransferSrc, 
                ::vk::MemoryPropertyFlagBits::eHostVisible | ::vk::MemoryPropertyFlagBits::eHostCoherent, buffer, bufferMemory);
    void* data = device.getVKDevice().mapMemory(bufferMemory, 0, buff_size);
    ::memcpy(data, bufferData, buff_size);
    device.getVKDevice().unmapMemory(bufferMemory);
}

StagingBuffer::~StagingBuffer()
{
    device_.get().destroyBuffer(buffer);
    device_.get().freeMemory(bufferMemory);
}

}