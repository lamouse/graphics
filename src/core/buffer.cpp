#include "buffer.hpp"

#include <cassert>

namespace core {

auto Buffer::getAlignmentSize(::vk::DeviceSize instanceSize, ::vk::DeviceSize minOffsetAlignment) -> ::vk::DeviceSize {
    if (minOffsetAlignment > 0) {
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
    }
    return instanceSize;
}

Buffer::Buffer(core::Device& device, ::vk::DeviceSize instanceSize, uint32_t instanceCount,
               ::vk::BufferUsageFlags bufferUsage, ::vk::MemoryPropertyFlags memoryPropertyFlags,
               ::vk::DeviceSize minOffsetAlignment)
    : device_(device),
      instanceCount_(instanceCount),
      instanceSize_(instanceSize),
      alignmentSize_(getAlignmentSize(instanceSize, minOffsetAlignment)), bufferUsage_(bufferUsage),
      memoryPropertyFlags_(memoryPropertyFlags) {

    bufferSize_ = instanceCount_ * instanceSize_;
    device.createBuffer(bufferSize_, bufferUsage_, memoryPropertyFlags_, buffer_, bufferMemory_);
}

Buffer::~Buffer() {
    unmap();
    device_.logicalDevice().destroyBuffer(buffer_);
    device_.logicalDevice().freeMemory(bufferMemory_);
}

void Buffer::map(::vk::DeviceSize size, ::vk::DeviceSize offset) {
    assert(buffer_ && bufferMemory_ && "Called map on buffer before create");
    data_ = device_.logicalDevice().mapMemory(bufferMemory_, offset, size);
}
void Buffer::unmap() {
    if (data_) {
        device_.logicalDevice().unmapMemory(bufferMemory_);
        data_ = nullptr;
    }
}

void Buffer::writeToBuffer(void* data, ::vk::DeviceSize size, ::vk::DeviceSize offset) {
    assert(data_ && "Can't copy to unmapped buffer");
    if (size == VK_WHOLE_SIZE) {
        memcpy(data_, data, bufferSize_);
    } else {
        char* memOffset = (char*)data_;
        memOffset += offset;
        memcpy(memOffset, data, size);
    }
}

auto Buffer::flush(::vk::DeviceSize size, ::vk::DeviceSize offset) -> ::vk::Result {
    ::vk::MappedMemoryRange flushRange{bufferMemory_, offset, size};
    return device_.logicalDevice().flushMappedMemoryRanges(1, &flushRange);
}

auto Buffer::invalidate(::vk::DeviceSize size, ::vk::DeviceSize offset) -> ::vk::Result {
    ::vk::MappedMemoryRange invalidateRange{bufferMemory_, offset, size};
    return device_.logicalDevice().invalidateMappedMemoryRanges(1, &invalidateRange);
}

auto Buffer::descriptorInfo(::vk::DeviceSize size, ::vk::DeviceSize offset) -> ::vk::DescriptorBufferInfo {
    return {buffer_, offset, size};
}

void Buffer::writeToIndex(void* data, ::vk::DeviceSize index) {
    writeToBuffer(data, instanceSize_, index * alignmentSize_);
}

auto Buffer::flushIndex(::vk::DeviceSize index) -> ::vk::Result {
    return flush(alignmentSize_, index * alignmentSize_);
}

auto Buffer::descriptorInfoForIndex(::vk::DeviceSize index) -> ::vk::DescriptorBufferInfo {
    return descriptorInfo(alignmentSize_, index * alignmentSize_);
}

auto Buffer::invalidateIndex(::vk::DeviceSize index) -> ::vk::Result {
    return invalidate(alignmentSize_, index * alignmentSize_);
}

}  // namespace core