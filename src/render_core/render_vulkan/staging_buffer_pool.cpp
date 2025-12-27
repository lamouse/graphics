module;
#include <algorithm>
#include "common/literals.hpp"
#include "common/bit_util.h"
#include <vulkan/vulkan.hpp>

#include "common/alignment.hpp"
#undef min
module render.vulkan.staging_buffer_pool;
import render.vulkan.common;
import render.vulkan.scheduler;

namespace render::vulkan {
namespace {
using namespace common::literals;
// Maximum potential alignment of a Vulkan buffer
constexpr vk::DeviceSize MAX_ALIGNMENT = 256;
// Stream buffer size in bytes
constexpr vk::DeviceSize MAX_STREAM_BUFFER_SIZE = 128_MiB;
auto GetStreamBufferSize(const Device& device) -> size_t {
    vk::DeviceSize size{0};
    if (device.hasDebuggingToolAttached()) {
        ForEachDeviceLocalHostVisibleHeap(device, [&size]([[maybe_unused]]size_t index, vk::MemoryHeap& heap) {
            size = std::max(size, heap.size);
        });
        // rebar Resizable Base Address Register
        //  If rebar is not supported, cut the max heap size to 40%. This will allow 2 captures to
        //  be loaded at the same time in RenderDoc. If rebar is supported, this shouldn't be an
        //  issue as the heap will be much larger.
        if (size <= 256_MiB) {
            size = size * 40 / 100;
        }
    } else {
        size = MAX_STREAM_BUFFER_SIZE;
    }
    return std::min(common::alignUp(size, MAX_ALIGNMENT), MAX_STREAM_BUFFER_SIZE);
}
}  // namespace

StagingBufferPool::StagingBufferPool(const Device& device_, MemoryAllocator& memory_allocator_,
                                     scheduler::Scheduler& scheduler_)
    : device{device_},
      memory_allocator{memory_allocator_},
      scheduler{scheduler_},
      stream_buffer_size{GetStreamBufferSize(device)},
      region_size{stream_buffer_size / StagingBufferPool::NUM_SYNCS} {
    VkBufferCreateInfo stream_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = stream_buffer_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    if (device.isExtTransformFeedbackSupported()) {
        stream_ci.usage |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    }
    stream_buffer = memory_allocator.createBuffer(stream_ci, MemoryUsage::Stream);
    if (device.hasDebuggingToolAttached()) {
        stream_buffer.SetObjectNameEXT("Stream Buffer");
    }
    stream_pointer = stream_buffer.Mapped();
    assert(!stream_pointer.empty() && "Stream buffer must be host visible!");
}
StagingBufferPool::~StagingBufferPool() = default;

auto StagingBufferPool::Request(size_t size, MemoryUsage usage, bool deferred) -> StagingBufferRef {
    if (!deferred && usage == MemoryUsage::Upload && size <= region_size) {
        return GetStreamBuffer(size);
    }
    return GetStagingBuffer(size, usage, deferred);
}
void StagingBufferPool::FreeDeferred(StagingBufferRef& ref) {
    auto& entries = GetCache(ref.usage)[ref.log2_level].entries;
    const auto is_this_one = [&ref](const StagingBuffer& entry) {
        return entry.index == ref.index;
    };
    auto it = std::ranges::find_if(entries, is_this_one);
    assert(it != entries.end());
    assert(it->deferred);
    it->tick = scheduler.currentTick();
    it->deferred = false;
}

void StagingBufferPool::TickFrame() {
    current_delete_level = (current_delete_level + 1) % NUM_LEVELS;
    ReleaseCache(MemoryUsage::DeviceLocal);
    ReleaseCache(MemoryUsage::Upload);
    ReleaseCache(MemoryUsage::Download);
}

auto StagingBufferPool::GetStreamBuffer(size_t size) -> StagingBufferRef {
    if (AreRegionsActive(Region(free_iterator) + 1,
                         std::min(Region(iterator + size) + 1, NUM_SYNCS))) {
        // Avoid waiting for the previous usages to be free
        return GetStagingBuffer(size, MemoryUsage::Upload);
    }
    const u64 current_tick = scheduler.currentTick();
    std::fill(sync_ticks.begin() + Region(used_iterator), sync_ticks.begin() + Region(iterator),
              current_tick);
    used_iterator = iterator;
    free_iterator = std::max(free_iterator, iterator + size);

    if (iterator + size >= stream_buffer_size) {
        std::fill(sync_ticks.begin() + Region(used_iterator), sync_ticks.begin() + NUM_SYNCS,
                  current_tick);
        used_iterator = 0;
        iterator = 0;
        free_iterator = size;

        if (AreRegionsActive(0, Region(size) + 1)) {
            // Avoid waiting for the previous usages to be free
            return GetStagingBuffer(size, MemoryUsage::Upload);
        }
    }
    const size_t offset = iterator;
    iterator = common::alignUp(iterator + size, MAX_ALIGNMENT);
    return StagingBufferRef{
        .buffer = *stream_buffer,
        .offset = static_cast<VkDeviceSize>(offset),
        .mapped_span = stream_pointer.subspan(offset, size),
        .usage{},
        .log2_level{},
        .index{},
    };
}

auto StagingBufferPool::AreRegionsActive(size_t region_begin, size_t region_end) const -> bool {
    const u64 gpu_tick = scheduler.getMasterSemaphore().knownGpuTick();
    return std::any_of(sync_ticks.begin() + region_begin, sync_ticks.begin() + region_end,
                       [gpu_tick](u64 sync_tick) { return gpu_tick < sync_tick; });
};

auto StagingBufferPool::GetStagingBuffer(size_t size, MemoryUsage usage, bool deferred)
    -> StagingBufferRef {
    if (const std::optional<StagingBufferRef> ref = TryGetReservedBuffer(size, usage, deferred)) {
        return *ref;
    }
    return CreateStagingBuffer(size, usage, deferred);
}

auto StagingBufferPool::TryGetReservedBuffer(size_t size, MemoryUsage usage, bool deferred)
    -> std::optional<StagingBufferRef> {
    StagingBuffers& cache_level = GetCache(usage)[common::Log2Ceil64(size)];

    const auto is_free = [this](const StagingBuffer& entry) {
        return !entry.deferred && scheduler.isFree(entry.tick);
    };
    auto& entries = cache_level.entries;
    const auto hint_it = entries.begin() + cache_level.iterate_index;
    auto it = std::find_if(entries.begin() + cache_level.iterate_index, entries.end(), is_free);
    if (it == entries.end()) {
        it = std::find_if(entries.begin(), hint_it, is_free);
        if (it == hint_it) {
            return std::nullopt;
        }
    }
    cache_level.iterate_index = std::distance(entries.begin(), it) + 1;
    it->tick = deferred ? std::numeric_limits<u64>::max() : scheduler.currentTick();
    assert(!it->deferred);
    it->deferred = deferred;
    return it->Ref();
}

auto StagingBufferPool::CreateStagingBuffer(size_t size, MemoryUsage usage, bool deferred)
    -> StagingBufferRef {
    const u32 log2 = common::Log2Ceil64(size);
    VkBufferCreateInfo buffer_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = 1ULL << log2,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    if (device.isExtTransformFeedbackSupported()) {
        buffer_ci.usage |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    }
    Buffer buffer = memory_allocator.createBuffer(buffer_ci, usage);
    if (device.hasDebuggingToolAttached()) {
        ++buffer_index;
        buffer.SetObjectNameEXT(std::format("Staging Buffer {}", buffer_index).c_str());
    }
    const std::span<u8> mapped_span = buffer.Mapped();
    StagingBuffer& entry = GetCache(usage)[log2].entries.emplace_back(StagingBuffer{
        .buffer = std::move(buffer),
        .mapped_span = mapped_span,
        .usage = usage,
        .log2_level = log2,
        .index = unique_ids++,
        .tick = deferred ? std::numeric_limits<u64>::max() : scheduler.currentTick(),
        .deferred = deferred,
    });
    return entry.Ref();
}

auto StagingBufferPool::GetCache(MemoryUsage usage) -> StagingBufferPool::StagingBuffersCache& {
    switch (usage) {
        case MemoryUsage::DeviceLocal:
            return device_local_cache;
        case MemoryUsage::Upload:
            return upload_cache;
        case MemoryUsage::Download:
            return download_cache;
        default:
            assert(false && "Invalid memory usage=");
            return upload_cache;
    }
}

void StagingBufferPool::ReleaseCache(MemoryUsage usage) {
    ReleaseLevel(GetCache(usage), current_delete_level);
}

void StagingBufferPool::ReleaseLevel(StagingBuffersCache& cache, size_t log2) {
    constexpr size_t deletions_per_tick = 16;
    auto& staging = cache[log2];
    auto& entries = staging.entries;
    const size_t old_size = entries.size();

    const auto is_deletable = [this](const StagingBuffer& entry) {
        return scheduler.isFree(entry.tick);
    };
    const size_t begin_offset = staging.delete_index;
    const size_t end_offset = std::min(begin_offset + deletions_per_tick, old_size);
    const auto begin = entries.begin() + begin_offset;
    const auto end = entries.begin() + end_offset;
    entries.erase(std::remove_if(begin, end, is_deletable), end);

    const size_t new_size = entries.size();
    staging.delete_index += deletions_per_tick;
    if (staging.delete_index >= new_size) {
        staging.delete_index = 0;
    }
    if (staging.iterate_index > new_size) {
        staging.iterate_index = 0;
    }
}

}  // namespace render::vulkan