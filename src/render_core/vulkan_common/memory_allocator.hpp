#pragma once

#include "common/common_types.hpp"
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include "render_core/vulkan_common/device.hpp"
VK_DEFINE_HANDLE(VmaAllocator)
namespace render::vulkan {
template <typename F>
void ForEachDeviceLocalHostVisibleHeap(const Device& device, F&& f) {
    auto memory_props = device.getPhysical().getMemoryProperties2().memoryProperties;
    for (size_t i = 0; i < memory_props.memoryTypeCount; i++) {
        auto& memory_type = memory_props.memoryTypes[i];
        if ((memory_type.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) &&
            (memory_type.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)) {
            f(memory_type.heapIndex, memory_props.memoryHeaps[memory_type.heapIndex]);
        }
    }
}

/// Hints and requirements for the backing memory type of a commit
enum class MemoryUsage {
    DeviceLocal,  ///< Requests device local host visible buffer, falling back to device local
                  ///< memory.
    Upload,       ///< Requires a host visible memory type optimized for CPU to GPU uploads
    Download,     ///< Requires a host visible memory type optimized for GPU to CPU readbacks
    Stream        ///< Requests device local host visible buffer, falling back host memory.
};

/// Memory allocator container.
/// Allocates and releases memory allocations on demand.
class MemoryAllocator {
        friend class MemoryAllocation;

    public:
        /**
         * Construct memory allocator
         *
         * @param device_             Device to allocate from
         *
         * @throw vk::Exception on failure
         */
        explicit MemoryAllocator(const Device& device_);
        ~MemoryAllocator();
        MemoryAllocator(const MemoryAllocator&) = delete;
        MemoryAllocator(MemoryAllocator&&) = delete;
        auto operator=(const MemoryAllocator&) = delete;
        auto operator=(MemoryAllocator&&) = delete;

        [[nodiscard]] auto createImage(const VkImageCreateInfo& ci) const -> Image;

        [[nodiscard]] auto createBuffer(const VkBufferCreateInfo& ci, MemoryUsage usage) const
            -> Buffer;

    private:
        const Device& device;                                 ///< Device handle.
        VmaAllocator allocator;                               ///< Vma allocator.
        const vk::PhysicalDeviceMemoryProperties properties;  ///< Physical device properties.
        VkDeviceSize buffer_image_granularity;  // The granularity for adjacent offsets between
                                                // buffers and optimal images
        u32 valid_memory_types{~0u};
};

}  // namespace render::vulkan