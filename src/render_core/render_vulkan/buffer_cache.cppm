module;
#include "render_core/buffer_cache/buffer_base.hpp"
#include "render_core/buffer_cache/buffer_cache.h"
#include "render_core/surface.hpp"
#include "render_core/texture/types.hpp"
#include "render_core/pipeline_state.h"
#include <vulkan/vulkan.hpp>
export module render.vulkan:buffer_cache;
import render.vulkan.common;
import render.vulkan.descriptor_pool;
import render.vulkan.scheduler;
import :staging_buffer_pool;
import :update_descriptor;


export namespace render::vulkan {

class BufferCacheRuntime;

class BaseBufferCache : public buffer::BufferBase {
    public:
        explicit BaseBufferCache(BufferCacheRuntime&, buffer::NullBufferParams null_params);
        explicit BaseBufferCache(BufferCacheRuntime& runtime, u64 size_bytes_);
        operator vk::Buffer() const noexcept { return *buffer; }
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
        bool is_null{};
};

class BufferCacheRuntime {
        friend class BaseBufferCache;

    public:
        explicit BufferCacheRuntime(const Device& device_, MemoryAllocator& memory_manager_,
                                    scheduler::Scheduler& scheduler_,
                                    StagingBufferPool& staging_pool_,
                                    GuestDescriptorQueue& guest_descriptor_queue,
                                    ComputePassDescriptorQueue& compute_pass_descriptor_queue);

        void TickFrame() noexcept;

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

        void CopyBuffer(vk::Buffer src_buffer, vk::Buffer dst_buffer,
                        std::span<const texture::BufferCopy> copies, bool barrier,
                        bool can_reorder_upload = false);

        void ClearBuffer(vk::Buffer dest_buffer, u32 offset, size_t size, u32 value);

        void BindIndexBuffer(IndexFormat index_format, vk::Buffer buffer);

        void BindVertexBuffer(u32 index, vk::Buffer buffer, u32 offset, u32 size, u32 stride);

        void BindVertexBuffers(std::unique_ptr<buffer::HostBindings<BaseBufferCache>>&& bindings);

        void BindTransformFeedbackBuffer(u32 index, vk::Buffer buffer, u32 offset, u32 size);

        void BindTransformFeedbackBuffers(buffer::HostBindings<BaseBufferCache>& bindings);

        auto BindMappedUniformBuffer([[maybe_unused]] size_t stage,
                                     [[maybe_unused]] u32 binding_index, u32 size)
            -> std::span<u8> {
            u32 offset = 0;
            if (auto span = uniform_ring.Alloc(size, offset); !span.empty()) {
                BindBuffer(*uniform_ring.buffers[uniform_ring.current_frame], offset, size);
                return span;
            }
            const StagingBufferRef ref = staging_pool.Request(size, MemoryUsage::Upload);
            BindBuffer(ref.buffer, static_cast<u32>(ref.offset), size);
            return ref.mapped_span;
        }

        void BindUniformBuffer(vk::Buffer buffer, u32 offset, u32 size) {
            BindBuffer(buffer, offset, size);
        }

        void BindStorageBuffer(vk::Buffer buffer, u32 offset, u32 size) {
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

        struct UniformRing {
                static constexpr size_t NUM_FRAMES = 3;
                std::array<Buffer, NUM_FRAMES> buffers{};
                std::array<u8*, NUM_FRAMES> mapped{};
                u64 size = 0;
                u64 head = 0;
                u32 align = 256;
                size_t current_frame = 0;

                void Init(const Device& device, MemoryAllocator& alloc, u64 bytes, u32 alignment);
                void BeginFrame() {
                    current_frame = (current_frame + 1) % NUM_FRAMES;
                    head = 0;
                }
                auto Alloc(u32 bytes, u32& out_offset) -> std::span<u8>;
        };
        UniformRing uniform_ring;

        const Device& device;
        MemoryAllocator& memory_allocator;
        scheduler::Scheduler& scheduler;
        StagingBufferPool& staging_pool;
        GuestDescriptorQueue& guest_descriptor_queue;

        Buffer null_buffer;
};

struct BufferCacheParams {
        using Runtime = render::vulkan::BufferCacheRuntime;
        using Buffer = render::vulkan::BaseBufferCache;
};

using BufferCache = render::buffer::BufferCache<BufferCacheParams>;

}  // namespace render::vulkan
