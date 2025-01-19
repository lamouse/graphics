#pragma once

#include "render_core/buffer_cache/buffer_base.hpp"
#include "render_core/buffer_cache/buffer_cache.h"
#include "common/slot_vector.hpp"
#include "compute_pass.hpp"
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include "render_core/surface.hpp"
#include "render_core/buffer_cache/usage_tracker.hpp"
#include "render_core/render_vulkan/staging_buffer_pool.hpp"
#include "render_core/render_vulkan/update_descriptor.hpp"
#include "render_core/texture/types.hpp"
#include "render_core/render_vulkan/fixed_pipeline_state.h"

namespace render::vulkan {
class Device;
class MemoryAllocator;
namespace scheduler {
class Scheduler;
}
namespace resource {
class DescriptorPool;
}
class BufferCacheRuntime;

class BaseBufferCache : public buffer::BufferBase {
    public:
        explicit BaseBufferCache(BufferCacheRuntime&, buffer::NullBufferParams null_params);
        explicit BaseBufferCache(BufferCacheRuntime& runtime, VAddr cpu_addr_, u64 size_bytes_);
        explicit operator vk::Buffer() const noexcept { return *buffer; }
        void ResetUsageTracking() noexcept { tracker.Reset(); }
        [[nodiscard]] auto IsRegionUsed(u64 offset, u64 size) const noexcept -> bool {
            return tracker.IsUsed(offset, size);
        }

        void MarkUsage(u64 offset, u64 size) noexcept { tracker.Track(offset, size); }
        [[nodiscard]] auto View(u32 offset, u32 size, surface::PixelFormat format)
            -> vk::BufferView;

        [[nodiscard]] auto Handle() const noexcept -> vk::Buffer { return *buffer; }

    private:
        struct BufferView {
                u32 offset;
                u32 size;
                surface::PixelFormat format;
                VulkanBufferView handle;
        };

        const Device* device{};
        Buffer buffer;
        std::vector<BufferView> views;
        buffer::UsageTracker tracker;
        bool is_null{};
};

class QuadArrayIndexBuffer;
class QuadStripIndexBuffer;

class BufferCacheRuntime {
        friend BaseBufferCache;

    public:
        explicit BufferCacheRuntime(const Device& device_, MemoryAllocator& memory_manager_,
                                    scheduler::Scheduler& scheduler_,
                                    StagingBufferPool& staging_pool_,
                                    GuestDescriptorQueue& guest_descriptor_queue,
                                    ComputePassDescriptorQueue& compute_pass_descriptor_queue,
                                    resource::DescriptorPool& descriptor_pool);

        void TickFrame(common::SlotVector<BaseBufferCache>& slot_buffers) noexcept;

        void Finish();

        [[nodiscard]] auto GetDeviceLocalMemory() const -> u64;

        [[nodiscard]] auto GetDeviceMemoryUsage() const -> u64;

        [[nodiscard]] auto CanReportMemoryUsage() const -> bool;

        [[nodiscard]] auto GetStorageBufferAlignment() const -> u32;

        [[nodiscard]] auto UploadStagingBuffer(size_t size) -> StagingBufferRef;

        [[nodiscard]] auto DownloadStagingBuffer(size_t size, bool deferred = false)
            -> StagingBufferRef;

        auto CanReorderUpload(const BaseBufferCache& buffer,
                              std::span<const texture::BufferCopy> copies) -> bool;

        void FreeDeferredStagingBuffer(StagingBufferRef& ref);

        void PreCopyBarrier();

        void CopyBuffer(vk::Buffer src_buffer, vk::Buffer dst_buffer,
                        std::span<const texture::BufferCopy> copies, bool barrier,
                        bool can_reorder_upload = false);

        void PostCopyBarrier();

        void ClearBuffer(VkBuffer dest_buffer, u32 offset, size_t size, u32 value);

        void BindIndexBuffer(PrimitiveTopology topology, IndexFormat index_format, u32 num_indices,
                             u32 base_vertex, vk::Buffer buffer, u32 offset, u32 size);

        void BindQuadIndexBuffer(PrimitiveTopology topology, u32 first, u32 count);

        void BindVertexBuffer(u32 index, vk::Buffer buffer, u32 offset, u32 size, u32 stride);

        void BindVertexBuffers(buffer::HostBindings<BaseBufferCache>& bindings);

        void BindTransformFeedbackBuffer(u32 index, vk::Buffer buffer, u32 offset, u32 size);

        void BindTransformFeedbackBuffers(buffer::HostBindings<BaseBufferCache>& bindings);

        auto BindMappedUniformBuffer([[maybe_unused]] size_t stage,
                                     [[maybe_unused]] u32 binding_index, u32 size)
            -> std::span<u8> {
            const StagingBufferRef ref = staging_pool.Request(size, MemoryUsage::Upload);
            BindBuffer(ref.buffer, static_cast<u32>(ref.offset), size);
            return ref.mapped_span;
        }

        void BindUniformBuffer(vk::Buffer buffer, u32 offset, u32 size) {
            BindBuffer(buffer, offset, size);
        }

        void BindStorageBuffer(vk::Buffer buffer, u32 offset, u32 size,
                               [[maybe_unused]] bool is_written) {
            BindBuffer(buffer, offset, size);
        }

        void BindTextureBuffer(BaseBufferCache& buffer, u32 offset, u32 size,
                               surface::PixelFormat format) {
            guest_descriptor_queue.AddTexelBuffer(buffer.View(offset, size, format));
        }

    private:
        void BindBuffer(vk::Buffer buffer, u32 offset, u32 size) {
            guest_descriptor_queue.AddBuffer(buffer, offset, size);
        }

        void ReserveNullBuffer();
        auto CreateNullBuffer() -> Buffer;

        const Device& device;
        MemoryAllocator& memory_allocator;
        scheduler::Scheduler& scheduler;
        StagingBufferPool& staging_pool;
        GuestDescriptorQueue& guest_descriptor_queue;

        std::shared_ptr<QuadArrayIndexBuffer> quad_array_index_buffer;
        std::shared_ptr<QuadStripIndexBuffer> quad_strip_index_buffer;

        Buffer null_buffer;

        std::unique_ptr<Uint8Pass> uint8_pass;
        QuadIndexedPass quad_index_pass;
};

struct BufferCacheParams {
        using Runtime = render::vulkan::BufferCacheRuntime;
        using Buffer = render::vulkan::BaseBufferCache;
        using Async_Buffer = render::vulkan::StagingBufferRef;

        static constexpr bool IS_OPENGL = false;
        static constexpr bool HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS = false;
        static constexpr bool HAS_FULL_INDEX_AND_PRIMITIVE_SUPPORT = false;
        static constexpr bool NEEDS_BIND_UNIFORM_INDEX = false;
        static constexpr bool NEEDS_BIND_STORAGE_INDEX = false;
        static constexpr bool USE_MEMORY_MAPS = true;
        static constexpr bool SEPARATE_IMAGE_BUFFER_BINDINGS = false;
        static constexpr bool USE_MEMORY_MAPS_FOR_UPLOADS = true;
};

using BufferCache = render::buffer::BufferCache<BufferCacheParams>;
}  // namespace render::vulkan