#pragma once

#include <memory>
#include <span>
#include <vector>
#include "common/common_types.hpp"
#include "common/common_funcs.hpp"
#include <vulkan/vulkan.hpp>
#include "vma.hpp"
import render.vulkan.common;

namespace render::vulkan {

class MemoryMap;
class MemoryAllocation;
/// Hints and requirements for the backing memory type of a commit
enum class MemoryUsage {
    DeviceLocal,  ///< Requests device local host visible buffer, falling back to device local
                  ///< memory.
    Upload,       ///< Requires a host visible memory type optimized for CPU to GPU uploads
    Download,     ///< Requires a host visible memory type optimized for GPU to CPU readbacks
    Stream        ///< Requests device local host visible buffer, falling back host memory.
};
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

/// Ownership handle of a memory commitment.
/// Points to a subregion of a memory allocation.
class MemoryCommit {
    public:
        explicit MemoryCommit() noexcept = default;
        explicit MemoryCommit(MemoryAllocation* allocation_, vk::DeviceMemory memory_, u64 begin_,
                              u64 end_) noexcept;
        ~MemoryCommit();

        auto operator=(MemoryCommit&&) noexcept -> MemoryCommit&;
        MemoryCommit(MemoryCommit&&) noexcept;

        auto operator=(const MemoryCommit&) -> MemoryCommit& = delete;
        MemoryCommit(const MemoryCommit&) = delete;

        /// Returns a host visible memory map.
        /// It will map the backing allocation if it hasn't been mapped before.
        auto map() -> std::span<u8>;

        /// Returns the Vulkan memory handler.
        [[nodiscard]] auto memory() const -> vk::DeviceMemory { return memory_; }

        /// Returns the start position of the commit relative to the allocation.
        [[nodiscard]] auto offset() const -> vk::DeviceSize {
            return static_cast<vk::DeviceSize>(begin_);
        }

    private:
        void release();

        MemoryAllocation* allocation_{};  ///< Pointer to the large memory allocation.
        vk::DeviceMemory memory_;         ///< Vulkan device memory handler.
        u64 begin_{};                     ///< Beginning offset in bytes to where the commit exists.
        u64 end_{};                       ///< Offset in bytes where the commit ends.
        std::span<u8> span_;  ///< Host visible memory span. Empty if not queried before.
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

        CLASS_NON_COPYABLE(MemoryAllocator);

        [[nodiscard]] auto createImage(const VkImageCreateInfo& ci) const -> Image;

        [[nodiscard]] auto createBuffer(const VkBufferCreateInfo& ci, MemoryUsage usage) const
            -> Buffer;

        /**
         * Commits a memory with the specified requirements.
         *
         * @param requirements Requirements returned from a Vulkan call.
         * @param usage        Indicates how the memory will be used.
         *
         * @returns A memory commit.
         */
        auto commit(const vk::MemoryRequirements& requirements, MemoryUsage usage) -> MemoryCommit;

        /// Commits memory required by the buffer and binds it.
        auto commit(const vk::Buffer& buffer, MemoryUsage usage) -> MemoryCommit;

    private:
        /// Tries to allocate a chunk of memory.
        auto tryAllocMemory(vk::MemoryPropertyFlags flags, u32 type_mask, u64 size) -> bool;

        /// Releases a chunk of memory.
        void releaseMemory(MemoryAllocation* alloc);

        /// Tries to allocate a memory commit.
        auto tryCommit(const vk::MemoryRequirements& requirements, vk::MemoryPropertyFlags flags)
            -> std::optional<MemoryCommit>;

        /// Returns the fastest compatible memory property flags from the wanted flags.
        [[nodiscard]] auto memoryPropertyFlags(u32 type_mask, vk::MemoryPropertyFlags flags) const
            -> vk::MemoryPropertyFlags;

        /// Returns index to the fastest memory type compatible with the passed requirements.
        [[nodiscard]] auto findType(vk::MemoryPropertyFlags flags, u32 type_mask) const
            -> std::optional<u32>;

        const Device& device;                                 ///< Device handle.
        VmaAllocator allocator;                               ///< Vma allocator.
        const vk::PhysicalDeviceMemoryProperties properties;  ///< Physical device properties.
        std::vector<std::unique_ptr<MemoryAllocation>> allocations;  ///< Current allocations.
        VkDeviceSize buffer_image_granularity;  // The granularity for adjacent offsets between
                                                // buffers and optimal images
        u32 valid_memory_types{~0u};
};

}  // namespace render::vulkan