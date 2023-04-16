#pragma once
#include "device.hpp"
namespace core {

    class Buffer
    {
        public:
            Buffer(core::Device& device, ::vk::DeviceSize instanceSize, uint32_t instanceCount, 
                ::vk::BufferUsageFlags bufferUsage, ::vk::MemoryPropertyFlags memoryPropertyFlags,
                ::vk::DeviceSize minOffsetAlignment = 1);
            ~Buffer();

            void map(::vk::DeviceSize size = VK_WHOLE_SIZE, ::vk::DeviceSize offset = 0);
            void unmap();
            void writeToBuffer(void *data, ::vk::DeviceSize size = VK_WHOLE_SIZE, ::vk::DeviceSize offset = 0);
            /**
             * @brief Flushes a memory of the buffer to make visible to the device
             * @note Only requires for non-coherent memory
             * @param size 
             * @param offset 
             */
            ::vk::Result flush(::vk::DeviceSize size = VK_WHOLE_SIZE, ::vk::DeviceSize offset = 0);
            /**
             * @brief Invalidates a memory range of the buffer it visible to the host
             * @note Only requires for non-coherent memory
             * @param size 
             * @param offset 
             * @return ::vk::Result 
             */
            ::vk::Result invalidate(::vk::DeviceSize size = VK_WHOLE_SIZE, ::vk::DeviceSize offset = 0);
            ::vk::DescriptorBufferInfo descriptorInfo(::vk::DeviceSize size = VK_WHOLE_SIZE, ::vk::DeviceSize offset = 0);

            void writeToIndex(void* data, int index);
            ::vk::Result flushIndex(int index);
            ::vk::DescriptorBufferInfo descriptorInfoForIndex(int index);
            ::vk::Result invalidateIndex(int index);

            ::vk::Buffer getBuffer()const {return buffer_;}
            void* getMappedMemory()const {return data_;}
            uint32_t getInstanceCount()const {return instanceCount_;}
            ::vk::DeviceSize getInstanceSize()const {return instanceSize_;}
            ::vk::DeviceSize getAlignmentSize()const {return instanceSize_;}
            ::vk::BufferUsageFlags getBufferUsage()const {return bufferUsage_;}
            ::vk::MemoryPropertyFlags getMemoryPropertyFlags()const {return memoryPropertyFlags_;}
            ::vk::DeviceSize getBufferSize()const {return bufferSize_;}
        private:
            /**
            * @brief returns the minimum instance required to compatible with devices minOffsetAlignment
            * 
            * @param instanceSize 
            * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member
            * @return ::vk::DeviceSize 
            */
            static ::vk::DeviceSize getAlignmentSize(::vk::DeviceSize instanceSize, ::vk::DeviceSize minOffsetAlignment);
            core::Device& device_;
            void* data_ = nullptr;
            ::vk::Buffer buffer_ = VK_NULL_HANDLE;
            ::vk::DeviceMemory bufferMemory_ = VK_NULL_HANDLE;

            ::vk::DeviceSize bufferSize_;
            uint32_t instanceCount_;
            ::vk::DeviceSize instanceSize_;
            ::vk::DeviceSize alignmentSize_;
            ::vk::BufferUsageFlags bufferUsage_;
            ::vk::MemoryPropertyFlags memoryPropertyFlags_;
    };

}