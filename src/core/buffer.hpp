#pragma once
#include "vulkan/vulkan.hpp"
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
namespace core {

class EXPORT Buffer {
    public:
        Buffer(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept;
        auto operator=(const Buffer&) -> Buffer& = delete;
        auto operator=(Buffer&&) noexcept -> Buffer&;
        ~Buffer();
        Buffer() = default;
        void map(::vk::DeviceSize size = VK_WHOLE_SIZE, ::vk::DeviceSize offset = 0);
        void unmap();
        void writeToBuffer(const void* data, ::vk::DeviceSize size = VK_WHOLE_SIZE,
                           ::vk::DeviceSize offset = 0);
        /**
         * @brief Flushes a memory of the buffer to make visible to the device
         * @note Only requires for non-coherent memory
         * @param size
         * @param offset
         */
        auto flush(::vk::DeviceSize size = VK_WHOLE_SIZE, ::vk::DeviceSize offset = 0)
            -> ::vk::Result;
        /**
         * @brief Invalidates a memory range of the buffer it visible to the host
         * @note Only requires for non-coherent memory
         * @param size
         * @param offset
         * @return ::vk::Result
         */
        auto invalidate(::vk::DeviceSize size = VK_WHOLE_SIZE, ::vk::DeviceSize offset = 0)
            -> ::vk::Result;
        auto descriptorInfo(::vk::DeviceSize size = VK_WHOLE_SIZE, ::vk::DeviceSize offset = 0)
            -> ::vk::DescriptorBufferInfo;

        void writeToIndex(void* data, ::vk::DeviceSize index);
        auto flushIndex(::vk::DeviceSize index) -> ::vk::Result;
        auto descriptorInfoForIndex(::vk::DeviceSize index) -> ::vk::DescriptorBufferInfo;
        auto invalidateIndex(::vk::DeviceSize index) -> ::vk::Result;

        [[nodiscard]] auto getBuffer() const -> ::vk::Buffer { return buffer_; }
        [[nodiscard]] auto getMappedMemory() const -> void* { return data_; }
        [[nodiscard]] auto getInstanceCount() const -> uint32_t { return instanceCount_; }
        [[nodiscard]] auto getInstanceSize() const -> ::vk::DeviceSize { return instanceSize_; }
        [[nodiscard]] auto getAlignmentSize() const -> ::vk::DeviceSize { return instanceSize_; }
        [[nodiscard]] auto getBufferUsage() const -> ::vk::BufferUsageFlags { return bufferUsage_; }
        [[nodiscard]] auto getMemoryPropertyFlags() const -> ::vk::MemoryPropertyFlags {
            return memoryPropertyFlags_;
        }
        [[nodiscard]] auto getBufferSize() const -> ::vk::DeviceSize { return bufferSize_; }
        friend class Device;

    private:
        Buffer(vk::Device device, ::vk::DeviceSize instanceSize, uint32_t instanceCount,
               ::vk::BufferUsageFlags bufferUsage, ::vk::MemoryPropertyFlags memoryPropertyFlags,
               ::vk::DeviceSize minOffsetAlignment = 1);
        /**
         * @brief returns the minimum instance required to compatible with devices
         * minOffsetAlignment
         *
         * @param instanceSize
         * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member
         * @return ::vk::DeviceSize
         */
        static auto getAlignmentSize(::vk::DeviceSize instanceSize,
                                     ::vk::DeviceSize minOffsetAlignment) -> ::vk::DeviceSize;
        void* data_ = nullptr;
        ::vk::Buffer buffer_ = VK_NULL_HANDLE;
        ::vk::DeviceMemory bufferMemory_ = VK_NULL_HANDLE;
        ::vk::Device device_;
        ::vk::DeviceSize bufferSize_;
        uint32_t instanceCount_;
        ::vk::DeviceSize instanceSize_;
        ::vk::DeviceSize alignmentSize_;
        ::vk::BufferUsageFlags bufferUsage_;
        ::vk::MemoryPropertyFlags memoryPropertyFlags_;
};

}  // namespace core