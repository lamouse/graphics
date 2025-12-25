#include "memory_allocator.hpp"
#include "common/alignment.h"
#include <spdlog/spdlog.h>
#include <optional>
#include <cassert>
#include "common/literals.hpp"
#include "vma.hpp"
import render.vulkan.common.wrapper;

namespace render::vulkan {
namespace {

struct Range {
        u64 begin;
        u64 end;

        [[nodiscard]] auto contains(u64 iterator, u64 size) const noexcept -> bool {
            return iterator < end && begin < iterator + size;
        }
};

[[nodiscard]] auto allocationChunkSize(u64 required_size) -> u64 {
    static constexpr std::array sizes{
        0x1000ULL << 10,  0x1400ULL << 10,  0x1800ULL << 10,  0x1c00ULL << 10, 0x2000ULL << 10,
        0x3200ULL << 10,  0x4000ULL << 10,  0x6000ULL << 10,  0x8000ULL << 10, 0xA000ULL << 10,
        0x10000ULL << 10, 0x18000ULL << 10, 0x20000ULL << 10,
    };
    static_assert(std::is_sorted(sizes.begin(), sizes.end()));

    const auto it = std::ranges::lower_bound(sizes, required_size);
    return it != sizes.end() ? *it : common::AlignUp(required_size, 4ULL << 20);
}
[[nodiscard]] auto memoryUsagePropertyFlags(MemoryUsage usage) -> vk::MemoryPropertyFlags {
    switch (usage) {
        case MemoryUsage::DeviceLocal:
            return vk::MemoryPropertyFlagBits::eDeviceLocal;
        case MemoryUsage::Upload:
            return vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent;
        case MemoryUsage::Download:
            return vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent |
                   vk::MemoryPropertyFlagBits::eHostCached;
        case MemoryUsage::Stream:
            return vk::MemoryPropertyFlagBits::eDeviceLocal |
                   vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent;
    }
    SPDLOG_ERROR("Invalid memory usage={}", static_cast<int>(usage));
    return vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
}

[[nodiscard]] auto memoryUsagePreferredVmaFlags(MemoryUsage usage) -> vk::MemoryPropertyFlags {
    return usage != MemoryUsage::DeviceLocal ? vk::MemoryPropertyFlagBits::eHostVisible
                                             : vk::MemoryPropertyFlags{};
}
[[nodiscard]] auto memoryUsageVmaFlags(MemoryUsage usage) -> VmaAllocationCreateFlags {
    switch (usage) {
        case MemoryUsage::Upload:
        case MemoryUsage::Stream:
            return VMA_ALLOCATION_CREATE_MAPPED_BIT |
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        case MemoryUsage::Download:
            return VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        case MemoryUsage::DeviceLocal:
            return {};
    }
    return {};
}

[[nodiscard]] auto memoryUsageVma(MemoryUsage usage) -> VmaMemoryUsage {
    switch (usage) {
        case MemoryUsage::DeviceLocal:
        case MemoryUsage::Stream:
            return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        case MemoryUsage::Upload:
        case MemoryUsage::Download:
            return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    }
    return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
}
}  // namespace

class MemoryAllocation {
    public:
        explicit MemoryAllocation(MemoryAllocator* const allocator_, DeviceMemory memory_,
                                  vk::MemoryPropertyFlags properties, u64 allocation_size_,
                                  u32 type)
            : allocator{allocator_},
              memory{std::move(memory_)},
              allocation_size{allocation_size_},
              property_flags{properties},
              shifted_memory_type{1U << type} {}

        CLASS_NON_COPYABLE(MemoryAllocation);
        CLASS_NON_MOVEABLE(MemoryAllocation);

        [[nodiscard]] auto commit(vk::DeviceSize size, vk::DeviceSize alignment)
            -> std::optional<MemoryCommit> {
            const std::optional<u64> alloc = findFreeRegion(size, alignment);
            if (!alloc) {
                // Signal out of memory, it'll try to do more allocations.
                return std::nullopt;
            }
            const Range range{
                .begin = *alloc,
                .end = *alloc + size,
            };
            commits.insert(std::ranges::upper_bound(commits, *alloc, {}, &Range::begin), range);
            return std::make_optional<MemoryCommit>(this, *memory, *alloc, *alloc + size);
        }

        void free(u64 begin) {
            const auto it = std::ranges::find(commits, begin, &Range::begin);
            assert(it != commits.end() && "Invalid commit");
            commits.erase(it);
            if (commits.empty()) {
                // Do not call any code involving 'this' after this call, the object will be
                // destroyed
                allocator->releaseMemory(this);
            }
        }

        [[nodiscard]] auto map() -> std::span<u8> {
            if (memory_mapped_span.empty()) {
                u8* data = memory.map(0, allocation_size);
                memory_mapped_span = std::span<u8>(data, allocation_size);
            }
            return memory_mapped_span;
        }

        /// Returns whether this allocation is compatible with the arguments.
        [[nodiscard]] auto isCompatible(vk::MemoryPropertyFlags flags, u32 type_mask) const
            -> bool {
            return (flags & property_flags) == flags && (type_mask & shifted_memory_type) != 0;
        }

    private:
        [[nodiscard]] static constexpr auto shiftType(u32 type) -> u32 { return 1U << type; }

        [[nodiscard]] auto findFreeRegion(u64 size, u64 alignment) noexcept -> std::optional<u64> {
            assert(std::has_single_bit(alignment));
            const u64 alignment_log2 = std::countr_zero(alignment);
            std::optional<u64> candidate;
            u64 iterator = 0;
            auto commit = commits.begin();
            while (iterator + size <= allocation_size) {
                candidate = candidate.value_or(iterator);
                if (commit == commits.end()) {
                    break;
                }
                if (commit->contains(*candidate, size)) {
                    candidate = std::nullopt;
                }
                iterator = common::AlignUpLog2(commit->end, alignment_log2);
                ++commit;
            }
            return candidate;
        }

        MemoryAllocator* const allocator;              ///< Parent memory allocation.
        const DeviceMemory memory;                     ///< Vulkan memory allocation handler.
        const u64 allocation_size;                     ///< Size of this allocation.
        const vk::MemoryPropertyFlags property_flags;  ///< Vulkan memory property flags.
        const u32 shifted_memory_type;                 ///< Shifted Vulkan memory type.
        std::vector<Range> commits;        ///< All commit ranges done from this allocation.
        std::span<u8> memory_mapped_span;  ///< Memory mapped span. Empty if not queried before.
};

MemoryCommit::MemoryCommit(MemoryAllocation* allocation, vk::DeviceMemory memory, u64 begin,
                           u64 end) noexcept
    : allocation_{allocation}, memory_{memory}, begin_{begin}, end_{end} {}

MemoryCommit::~MemoryCommit() { release(); }
auto MemoryCommit::operator=(MemoryCommit&& rhs) noexcept -> MemoryCommit& {
    release();
    allocation_ = std::exchange(rhs.allocation_, nullptr);
    memory_ = rhs.memory_;
    begin_ = rhs.begin_;
    end_ = rhs.end_;
    span_ = std::exchange(rhs.span_, std::span<u8>{});
    return *this;
}

MemoryCommit::MemoryCommit(MemoryCommit&& rhs) noexcept
    : allocation_{std::exchange(rhs.allocation_, nullptr)},
      memory_{rhs.memory_},
      begin_{rhs.begin_},
      end_{rhs.end_},
      span_{std::exchange(rhs.span_, std::span<u8>{})} {}

auto MemoryCommit::map() -> std::span<u8> {
    if (span_.empty()) {
        span_ = allocation_->map().subspan(begin_, end_ - begin_);
    }
    return span_;
}

void MemoryCommit::release() {
    if (allocation_) {
        allocation_->free(begin_);
    }
}
MemoryAllocator::MemoryAllocator(const Device& device_)
    : device{device_},
      allocator{device_.getAllocator()},
      properties{device_.getPhysical().getMemoryProperties2().memoryProperties},
      buffer_image_granularity{
          device_.getPhysical().getProperties().limits.bufferImageGranularity} {
    // GPUs not supporting rebar may only have a region with less than 256MB host visible/device
    // local memory. In that case, opening 2 RenderDoc captures side-by-side is not possible due to
    // the heap running out of memory. With RenderDoc attached and only a small host/device region,
    // only allow the stream buffer in this memory heap.
    if (device_.hasDebuggingToolAttached()) {
        using namespace common::literals;
        ForEachDeviceLocalHostVisibleHeap(device, [this](size_t index, VkMemoryHeap& heap) {
            if (heap.size <= 256_MiB) {
                valid_memory_types &= ~(1u << index);
            }
        });
    }
}

MemoryAllocator::~MemoryAllocator() = default;

auto MemoryAllocator::createImage(const VkImageCreateInfo& ci) const -> Image {
    const VmaAllocationCreateInfo alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = 0,
        .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .memoryTypeBits = 0,
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr,
        .priority = 0.f,
    };

    VkImage handle{};
    VmaAllocation allocation{};

    utils::check(static_cast<vk::Result>(
        vmaCreateImage(allocator, &ci, &alloc_ci, &handle, &allocation, nullptr)));

    return Image{handle, static_cast<vk::ImageUsageFlags>(ci.usage), device.getLogical(), allocator,
                 allocation};
}

auto MemoryAllocator::createBuffer(const VkBufferCreateInfo& ci, MemoryUsage usage) const
    -> Buffer {
    const VmaAllocationCreateInfo alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT | memoryUsageVmaFlags(usage),
        .usage = memoryUsageVma(usage),
        .requiredFlags = 0,
        .preferredFlags = static_cast<VkMemoryPropertyFlags>(memoryUsagePreferredVmaFlags(usage)),
        .memoryTypeBits = usage == MemoryUsage::Stream ? 0u : valid_memory_types,
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr,
        .priority = 0.f,
    };

    VkBuffer handle{};
    VmaAllocationInfo alloc_info{};
    VmaAllocation allocation{};
    VkMemoryPropertyFlags property_flags{};

    utils::check(vmaCreateBuffer(allocator, &ci, &alloc_ci, &handle, &allocation, &alloc_info));
    vmaGetAllocationMemoryProperties(allocator, allocation, &property_flags);

    u8* data = reinterpret_cast<u8*>(alloc_info.pMappedData);
    const std::span<u8> mapped_data = data ? std::span<u8>{data, ci.size} : std::span<u8>{};
    const bool is_coherent = property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return Buffer(handle, device.getLogical(), allocator, allocation, mapped_data, is_coherent);
}

auto MemoryAllocator::commit(const vk::MemoryRequirements& requirements, MemoryUsage usage)
    -> MemoryCommit {
    // Find the fastest memory flags we can afford with the current requirements
    const u32 type_mask = requirements.memoryTypeBits;
    const vk::MemoryPropertyFlags usage_flags = memoryUsagePropertyFlags(usage);
    const vk::MemoryPropertyFlags flags = memoryPropertyFlags(type_mask, usage_flags);
    if (std::optional<MemoryCommit> commit = tryCommit(requirements, flags)) {
        return std::move(*commit);
    }
    // Commit has failed, allocate more memory.
    const u64 chunk_size = allocationChunkSize(requirements.size);
    if (!tryAllocMemory(flags, type_mask, chunk_size)) {
        // TODO(Rodrigo): Handle out of memory situations in some way like flushing to guest memory.
        throw utils::VulkanException(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    }
    // Commit again, this time it won't fail since there's a fresh allocation above.
    // If it does, there's a bug.
    return tryCommit(requirements, flags).value();
}

auto MemoryAllocator::tryAllocMemory(vk::MemoryPropertyFlags flags, u32 type_mask, u64 size)
    -> bool {
    const u32 type = findType(flags, type_mask).value();
    DeviceMemory memory = device.logical().tryAllocateMemory({
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = size,
        .memoryTypeIndex = type,
    });
    if (!memory) {
        if ((flags & vk::MemoryPropertyFlagBits::eDeviceLocal) ==
            vk::MemoryPropertyFlagBits::eDeviceLocal) {
            // Try to allocate non device local memory
            return tryAllocMemory(flags & ~vk::MemoryPropertyFlagBits::eDeviceLocal, type_mask,
                                  size);
        } else {
            // RIP
            return false;
        }
    }
    allocations.push_back(
        std::make_unique<MemoryAllocation>(this, std::move(memory), flags, size, type));
    return true;
}

void MemoryAllocator::releaseMemory(MemoryAllocation* alloc) {
    const auto it = std::ranges::find(allocations, alloc, &std::unique_ptr<MemoryAllocation>::get);
    assert(it != allocations.end());
    allocations.erase(it);
}
auto MemoryAllocator::tryCommit(const vk::MemoryRequirements& requirements,
                                vk::MemoryPropertyFlags flags) -> std::optional<MemoryCommit> {
    for (auto& allocation : allocations) {
        if (!allocation->isCompatible(flags, requirements.memoryTypeBits)) {
            continue;
        }
        if (auto commit = allocation->commit(requirements.size, requirements.alignment)) {
            return commit;
        }
    }
    if ((flags & vk::MemoryPropertyFlagBits::eDeviceLocal) ==
        vk::MemoryPropertyFlagBits::eDeviceLocal) {
        // Look for non device local commits on failure
        return tryCommit(requirements, flags & ~vk::MemoryPropertyFlagBits::eDeviceLocal);
    }
    return std::nullopt;
}

auto MemoryAllocator::memoryPropertyFlags(u32 type_mask, vk::MemoryPropertyFlags flags) const
    -> vk::MemoryPropertyFlags {
    if (findType(flags, type_mask)) {
        // Found a memory type with those requirements
        return flags;
    }
    if ((flags & vk::MemoryPropertyFlagBits::eHostCached) ==
        vk::MemoryPropertyFlagBits::eHostCached) {
        // Remove host cached bit in case it's not supported
        return memoryPropertyFlags(type_mask, flags & ~vk::MemoryPropertyFlagBits::eHostCached);
    }
    if ((flags & vk::MemoryPropertyFlagBits::eDeviceLocal) ==
        vk::MemoryPropertyFlagBits::eDeviceLocal) {
        // Remove device local, if it's not supported by the requested resource
        return memoryPropertyFlags(type_mask, flags & ~vk::MemoryPropertyFlagBits::eDeviceLocal);
    }
    assert(false && "No compatible memory types found");
    return {};
}

auto MemoryAllocator::findType(vk::MemoryPropertyFlags flags, u32 type_mask) const
    -> std::optional<u32> {
    for (u32 type_index = 0; type_index < properties.memoryTypeCount; ++type_index) {
        const vk::MemoryPropertyFlags type_flags = properties.memoryTypes[type_index].propertyFlags;
        if ((type_mask & (1U << type_index)) != 0 && (type_flags & flags) == flags) {
            // The type matches in type and in the wanted properties.
            return type_index;
        }
    }
    // Failed to find index
    return std::nullopt;
}

}  // namespace render::vulkan