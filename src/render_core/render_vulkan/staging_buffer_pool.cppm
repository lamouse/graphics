module;
#include <climits>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "common/common_types.hpp"
export module render.vulkan:staging_buffer_pool;
import render.vulkan.common;
import render.vulkan.scheduler;

export namespace render::vulkan {


struct StagingBufferRef {
        vk::Buffer buffer;
        vk::DeviceSize offset;
        std::span<u8> mapped_span;
        MemoryUsage usage;
        u32 log2_level;
        u64 index;
};

class StagingBufferPool {
    public:
        static constexpr size_t NUM_SYNCS = 16;

        explicit StagingBufferPool(const Device& device, MemoryAllocator& memory_allocator,
                                   scheduler::Scheduler& scheduler);
        ~StagingBufferPool();

        auto Request(size_t size, MemoryUsage usage, bool deferred = false) -> StagingBufferRef;
        void FreeDeferred(StagingBufferRef& ref);

        [[nodiscard]] auto StreamBuf() const noexcept -> vk::Buffer { return *stream_buffer; }

        void TickFrame();

    private:
        struct StreamBufferCommit {
                size_t upper_bound;
                u64 tick;
        };

        struct StagingBuffer {
                Buffer buffer;
                std::span<u8> mapped_span;
                MemoryUsage usage;
                u32 log2_level;
                u64 index;
                u64 tick = 0;
                bool deferred{};

                [[nodiscard]] auto Ref() const noexcept -> StagingBufferRef {
                    return {
                        .buffer = *buffer,
                        .offset = 0,
                        .mapped_span = mapped_span,
                        .usage = usage,
                        .log2_level = log2_level,
                        .index = index,
                    };
                }
        };

        struct StagingBuffers {
                std::vector<StagingBuffer> entries;
                size_t delete_index = 0;
                size_t iterate_index = 0;
        };

        static constexpr size_t NUM_LEVELS = sizeof(size_t) * CHAR_BIT;
        using StagingBuffersCache = std::array<StagingBuffers, NUM_LEVELS>;

        auto GetStreamBuffer(size_t size) -> StagingBufferRef;

        [[nodiscard]] auto AreRegionsActive(size_t region_begin, size_t region_end) const -> bool;

        auto GetStagingBuffer(size_t size, MemoryUsage usage, bool deferred = false)
            -> StagingBufferRef;

        auto TryGetReservedBuffer(size_t size, MemoryUsage usage, bool deferred)
            -> std::optional<StagingBufferRef>;

        auto CreateStagingBuffer(size_t size, MemoryUsage usage, bool deferred) -> StagingBufferRef;

        auto GetCache(MemoryUsage usage) -> StagingBuffersCache&;

        void ReleaseCache(MemoryUsage usage);

        void ReleaseLevel(StagingBuffersCache& cache, size_t log2);
        [[nodiscard]] auto Region(size_t iter) const noexcept -> size_t {
            return iter / region_size;
        }

        const Device& device;
        MemoryAllocator& memory_allocator;
        scheduler::Scheduler& scheduler;

        Buffer stream_buffer;
        std::span<u8> stream_pointer;
        vk::DeviceSize stream_buffer_size;
        vk::DeviceSize region_size;

        size_t iterator = 0;
        size_t used_iterator = 0;
        size_t free_iterator = 0;
        std::array<u64, NUM_SYNCS> sync_ticks{};

        StagingBuffersCache device_local_cache;
        StagingBuffersCache upload_cache;
        StagingBuffersCache download_cache;

        size_t current_delete_level = 0;
        u64 buffer_index = 0;
        u64 unique_ids{};
};

}  // namespace render::vulkan