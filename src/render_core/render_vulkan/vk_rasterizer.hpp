#pragma once
#include "rasterizer_interface.hpp"
#include "core/frontend/window.hpp"
#include "vulkan_common/device.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"
#include "vulkan_common/memory_allocator.hpp"
#include "scheduler.hpp"
#include "texture_cache.hpp"
#include "descriptor_pool.hpp"
#include "staging_buffer_pool.hpp"
#include "texture/types.hpp"
#include "blit_image.hpp"
namespace render::vulkan {

class RasterizerVulkan final : public render::RasterizerInterface {
    public:
        explicit RasterizerVulkan(core::frontend::BaseWindow& emu_window_, const Device& device_,
                                  MemoryAllocator& memory_allocator_,
                                  scheduler::Scheduler& scheduler_);
        ~RasterizerVulkan() override;

        void Draw(bool is_indexed, u32 instance_count) override;
        void DrawIndirect() override;
        void DrawTexture() override;
        void Clear(u32 layer_count) override;
        void DispatchCompute() override;

        void BindGraphicsUniformBuffer(size_t stage, u32 index, GPUVAddr gpu_addr,
                                       u32 size) override;
        void DisableGraphicsUniformBuffer(size_t stage, u32 index) override;
        void FlushAll() override;
        void FlushRegion(DAddr addr, u64 size, CacheType which = CacheType::All) override;
        auto MustFlushRegion(DAddr addr, u64 size, CacheType which = CacheType::All)
            -> bool override;
        auto GetFlushArea(DAddr addr, u64 size) -> RasterizerDownloadArea override;
        void InvalidateRegion(DAddr addr, u64 size, CacheType which = CacheType::All) override;
        void InnerInvalidation(std::span<const std::pair<DAddr, std::size_t>> sequences) override;
        void OnCacheInvalidation(DAddr addr, u64 size) override;
        auto OnCPUWrite(DAddr addr, u64 size) -> bool override;
        void InvalidateGPUCache() override;
        void UnmapMemory(DAddr addr, u64 size) override;
        void ModifyGPUMemory(size_t as_id, GPUVAddr addr, u64 size) override;
        void SignalFence(std::function<void()>&& func) override;
        void SyncOperation(std::function<void()>&& func) override;
        void SignalSyncPoint(u32 value) override;
        void SignalReference() override;
        void ReleaseFences(bool force = true) override;
        void FlushAndInvalidateRegion(DAddr addr, u64 size,
                                      CacheType which = CacheType::All) override;
        void WaitForIdle() override;
        void FragmentBarrier() override;
        void TiledCacheBarrier() override;
        void FlushCommands() override;
        void TickFrame() override;
        auto AccelerateConditionalRendering() -> bool override;
        void AccelerateInlineToMemory(GPUVAddr address, size_t copy_size,
                                      std::span<const u8> memory) override;

        void ReleaseChannel(s32 channel_id) override;

    private:
        static constexpr size_t MAX_TEXTURES = 192;
        static constexpr size_t MAX_IMAGES = 48;
        static constexpr size_t MAX_IMAGE_VIEWS = MAX_TEXTURES + MAX_IMAGES;

        static constexpr vk::DeviceSize DEFAULT_BUFFER_SIZE = 4 * sizeof(float);

        template <typename Func>
        void PrepareDraw(bool is_indexed, Func&&);

        void FlushWork();

        void UpdateDynamicStates();

        void HandleTransformFeedback();

        const Device& device;
        MemoryAllocator& memory_allocator;
        scheduler::Scheduler& scheduler;
        BlitImageHelper blit_image;

        StagingBufferPool staging_pool;
        resource::DescriptorPool descriptor_pool;
        GuestDescriptorQueue guest_descriptor_queue;
        ComputePassDescriptorQueue compute_pass_descriptor_queue;
        RenderPassCache render_pass_cache;
        // TODO 各种cache
        TextureCacheRuntime texture_cache_runtime;
        PipelineCache pipeline_cache;

        vk::Event wfi_event;

        boost::container::static_vector<u32, MAX_IMAGE_VIEWS> image_view_indices;
        std::array<render::texture::ImageViewId, MAX_IMAGE_VIEWS> image_view_ids;
        boost::container::static_vector<vk::Sampler, MAX_TEXTURES> sampler_handles;

        u32 draw_counter = 0;
};

}  // namespace render::vulkan
