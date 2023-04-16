#include "buffer.hpp"
#include <cassert>
namespace core {


::vk::DeviceSize Buffer::getAlignmentSize(::vk::DeviceSize instanceSize, ::vk::DeviceSize minOffsetAlignment)
{
    if(minOffsetAlignment > 0)
    {
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
    }
    return instanceSize;
}

Buffer::Buffer(core::Device& device, ::vk::DeviceSize instanceSize, uint32_t instanceCount, 
                ::vk::BufferUsageFlags bufferUsage, ::vk::MemoryPropertyFlags memoryPropertyFlags,
                ::vk::DeviceSize minOffsetAlignment):device_(device), instanceSize_(instanceSize),
                instanceCount_(instanceCount), bufferUsage_(bufferUsage), memoryPropertyFlags_(memoryPropertyFlags)
{
    alignmentSize_ = getAlignmentSize(instanceSize_, minOffsetAlignment);
    bufferSize_ = instanceCount_ * instanceSize_;
    device.createBuffer(bufferSize_, bufferUsage_, memoryPropertyFlags_, buffer_, bufferMemory_);
}

Buffer::~Buffer()
{   
    unmap();
    device_.logicalDevice().destroyBuffer(buffer_);
    device_.logicalDevice().freeMemory(bufferMemory_);
}

void Buffer::map(::vk::DeviceSize size, ::vk::DeviceSize offset)
{
    assert(buffer_ && bufferMemory_ && "Called map on buffer before create");
    data_ = device_.logicalDevice().mapMemory(bufferMemory_, offset, size);
}
void Buffer::unmap()
{
    if (data_) {
        device_.logicalDevice().unmapMemory(bufferMemory_);
        data_ = nullptr;
    }
}


void Buffer::writeToBuffer(void *data, ::vk::DeviceSize size, ::vk::DeviceSize offset)
{
    assert(data_ && "Can't copy to unmapped buffer");
    if(size == VK_WHOLE_SIZE)
    {
        memcpy(data_, data, bufferSize_);
    }else {
        char *memOffset = (char*)data_;
        memOffset += offset;
        memcpy(memOffset, data, size);
    }
}

::vk::Result Buffer::flush(::vk::DeviceSize size, ::vk::DeviceSize offset)
{
    ::vk::MappedMemoryRange flushRange{bufferMemory_, offset, size};
    return device_.logicalDevice().flushMappedMemoryRanges(1, &flushRange);
}

::vk::Result Buffer::invalidate(::vk::DeviceSize size, ::vk::DeviceSize offset)
{
    ::vk::MappedMemoryRange invalidateRange{bufferMemory_, offset, size};
    return device_.logicalDevice().invalidateMappedMemoryRanges(1, &invalidateRange);
}

::vk::DescriptorBufferInfo Buffer::descriptorInfo(::vk::DeviceSize size, ::vk::DeviceSize offset)
{
    return {buffer_, offset, size};
}

void Buffer::writeToIndex(void* data, int index)
{
    writeToBuffer(data, instanceSize_, index * alignmentSize_);
}

::vk::Result Buffer::flushIndex(int index)
{
    return flush(alignmentSize_, index * alignmentSize_);
}

::vk::DescriptorBufferInfo Buffer::descriptorInfoForIndex(int index)
{
    return descriptorInfo(alignmentSize_, index * alignmentSize_);
}

::vk::Result Buffer::invalidateIndex(int index)
{
    return invalidate(alignmentSize_, index * alignmentSize_);
}

}