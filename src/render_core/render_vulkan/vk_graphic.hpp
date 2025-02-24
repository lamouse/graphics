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
#include "render_core/render_vulkan/vk_imgui.hpp"
#include "render_core/graphic.hpp"
#include "core/frontend/window.hpp"
#include "common/common_funcs.hpp"
namespace render::vulkan {
struct RenderTargetInfo {
        u32 indices_size;
        buffer::BufferId indices_buffer_id;
        buffer::BufferId vertex_buffer_id;
        u32 vertex_size;
        buffer::BufferId uniform_buffer_id;
        u32 uniform_buffer_size;
        texture::ImageViewId image_view_id;
        texture::SamplerId sampler_id;
        RenderTargetInfo() = default;
        RenderTargetInfo(u32 indices_size, buffer::BufferId indices_buffer_id,
                         buffer::BufferId vertex_buffer_id, buffer::BufferId uniform_buffer_id,
                         texture::ImageViewId image_view_id, texture::SamplerId sampler_id);
        CLASS_DEFAULT_MOVEABLE(RenderTargetInfo);
        CLASS_DEFAULT_COPYABLE(RenderTargetInfo);
};
struct FramebufferTextureInfo;
class Device;

class VulkanGraphics : public render::Graphic {
    public:
        explicit VulkanGraphics(core::frontend::BaseWindow* emu_window_, const Device& device_,
                                MemoryAllocator& memory_allocator_,
                                scheduler::Scheduler& scheduler_, ShaderNotify& shader_notify_,
                                ImguiCore* imgui);

        CLASS_NON_COPYABLE(VulkanGraphics);
        CLASS_NON_MOVEABLE(VulkanGraphics);
        void start() override;
        void setPipelineState(const PipelineState& state) override;
        void drawIndics(u32 indicesSize) override;
        void drawImgui(vk::CommandBuffer cmd_buf);
        auto addGraphicContext(const GraphicsContext& context) -> GraphicsId override;
        void bindUniformBuffer(GraphicsId id, void* data, size_t size) override;
        void draw(GraphicsId id) override;
        void end() override {};
        auto getDrawImage() -> ImTextureID override;
        ~VulkanGraphics() override;

        auto AccelerateDisplay(const frame::FramebufferConfig& config,
                               u32 pixel_stride) -> std::optional<FramebufferTextureInfo>;
        void TickFrame();

    private:
        static constexpr size_t MAX_TEXTURES = 192;
        static constexpr size_t MAX_IMAGES = 48;
        static constexpr size_t MAX_IMAGE_VIEWS = MAX_TEXTURES + MAX_IMAGES;

        static constexpr vk::DeviceSize DEFAULT_BUFFER_SIZE = 4 * sizeof(float);
        template <typename Func>
        void PrepareDraw(bool is_indexed, Func&&);
        void FlushWork();
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
        void UpdateBlendConstants();
        void UpdateDepthBias();
        void UpdateDepthBounds();
        void UpdateStencilFaces();
        void UpdateLineWidth();
        void clear();
        const Device& device;
        MemoryAllocator& memory_allocator;
        scheduler::Scheduler& scheduler;
        ImguiCore* imgui;
        Sampler sampler;
        StagingBufferPool staging_pool;
        resource::DescriptorPool descriptor_pool;
        GuestDescriptorQueue guest_descriptor_queue;
        ComputePassDescriptorQueue compute_pass_descriptor_queue;
        BlitImageHelper blit_image;
        RenderPassCache render_pass_cache;
        core::frontend::BaseWindow* emu_window;
        TextureCacheRuntime texture_cache_runtime;
        TextureCache texture_cache;
        BufferCacheRuntime buffer_cache_runtime;
        BufferCache buffer_cache;
        buffer::BufferId uniform_buffer_id;
        PipelineCache pipeline_cache;
        Event wfi_event;
        PipelineState pipeline_state;
        u32 draw_counter = 0;
        std::unordered_map<GraphicsId, RenderTargetInfo> draw_indices;
        common::SlotVector<RenderTargetInfo> slot_graphics;
        std::unordered_map<VkImageView, ImTextureID> imgui_textures;
};

}  // namespace render::vulkan