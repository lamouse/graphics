#pragma once
#include "render_core/vulkan_common/memory_allocator.hpp"
#include "render_core/render_vulkan/pipeline_cache.hpp"
#include "render_core/render_vulkan/scheduler.hpp"
#include "render_core/render_vulkan/descriptor_pool.hpp"
#include "render_core/render_vulkan/staging_buffer_pool.hpp"
#include "render_core/render_vulkan/render_pass.hpp"
#include "render_core/texture/types.hpp"
#include "render_core/render_vulkan/texture_cache.hpp"
#include "render_core/render_vulkan/buffer_cache.h"
#include "render_core/render_vulkan/blit_image.hpp"
#include "render_core/graphic.hpp"
#include "core/frontend/window.hpp"
#include "common/common_funcs.hpp"
namespace render::vulkan {

struct FramebufferTextureInfo;
class Device;

class VulkanGraphics : public render::Graphic {
    public:
        explicit VulkanGraphics(core::frontend::BaseWindow* emu_window_, const Device& device_,
                                MemoryAllocator& memory_allocator_,
                                scheduler::Scheduler& scheduler_);

        CLASS_NON_COPYABLE(VulkanGraphics);
        CLASS_NON_MOVEABLE(VulkanGraphics);
        void start() override {};
        void addTexture(const texture::ImageInfo& imageInfo) override;
        void addVertex(std::span<float> vertex, const ::std::span<uint16_t>& indices) override;
        void addUniformBuffer(void* data, size_t size) override;
        void drawIndics(u32 indicesSize) override;
        void end() override {};
        ~VulkanGraphics() override;

        auto AccelerateDisplay(const frame::FramebufferConfig& config,
                               u32 pixel_stride) -> std::optional<FramebufferTextureInfo>;

    private:
        static constexpr size_t MAX_TEXTURES = 192;
        static constexpr size_t MAX_IMAGES = 48;
        static constexpr size_t MAX_IMAGE_VIEWS = MAX_TEXTURES + MAX_IMAGES;

        static constexpr vk::DeviceSize DEFAULT_BUFFER_SIZE = 4 * sizeof(float);
        void UpdateDynamicStates();
        void UpdatePrimitiveRestartEnable();
        void UpdateRasterizerDiscardEnable();
        void UpdateDepthBiasEnable();
        void UpdateVertexInput();
        void UpdateCullMode();
        void UpdateDepthCompareOp();
        void UpdateFrontFace();
        void UpdateStencilOp();
        void UpdateDepthBoundsTestEnable();
        void UpdateDepthTestEnable();
        void UpdateDepthWriteEnable();
        void UpdateStencilTestEnable();
        void UpdateLogicOpEnable();
        void UpdateDepthClampEnable();
        void UpdateLogicOp();
        void UpdateBlending();
        void UpdateViewportsState();
        void UpdateScissorsState();
        const Device& device;
        MemoryAllocator& memory_allocator;
        scheduler::Scheduler& scheduler;

        StagingBufferPool staging_pool;
        resource::DescriptorPool descriptor_pool;
        GuestDescriptorQueue guest_descriptor_queue;
        ComputePassDescriptorQueue compute_pass_descriptor_queue;
        BlitImageHelper blit_image;
        RenderPassCache render_pass_cache;

        TextureCacheRuntime texture_cache_runtime;
        TextureCache texture_cache;
        BufferCacheRuntime buffer_cache_runtime;
        BufferCache buffer_cache;
        texture::ImageViewId image_view_id;
        texture::SamplerId sampler_id;
        buffer::BufferId uniform_buffer_id;
        PipelineCache pipeline_cache;
        Event wfi_event;
        boost::container::static_vector<u32, MAX_IMAGE_VIEWS> image_view_indices;
        std::array<texture::ImageViewId, MAX_IMAGE_VIEWS> image_view_ids;
        boost::container::static_vector<vk::Sampler, MAX_TEXTURES> sampler_handles;
        u32 draw_counter = 0;
};

}  // namespace render::vulkan