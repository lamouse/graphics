#pragma once
#include "render_core/render_vulkan/pipeline_cache.hpp"
#include "render_core/render_vulkan/scheduler.hpp"
#include "render_core/render_vulkan/descriptor_pool.hpp"
#include "render_core/render_vulkan/staging_buffer_pool.hpp"
#include "render_core/render_vulkan/render_pass.hpp"
#include "render_core/render_vulkan/texture_cache.hpp"
#include "render_core/render_vulkan/buffer_cache.h"
#include "render_core/render_vulkan/vk_imgui.hpp"
#include "render_core/graphic.hpp"
#include "core/frontend/window.hpp"
#include "common/class_traits.hpp"
#include "render_core/framebuffer_config.hpp"
import render.vulkan.common;
namespace render::vulkan {
using ModelId = common::SlotId;

struct FramebufferTextureInfo;

struct ModelResource {
        BufferId vertex_buffer_id{};
        std::size_t vertex_size{};
        std::size_t vertex_count{};

        u32 indices_count{};
        BufferId indices_buffer_id{};

        VertexAttributeId vertex_attribute_id{};
        VertexBindingsId vertex_binding_id{};
};

class VulkanGraphics : public render::Graphic {
    public:
        explicit VulkanGraphics(core::frontend::BaseWindow* emu_window_, const Device& device_,
                                MemoryAllocator& memory_allocator_,
                                scheduler::Scheduler& scheduler_, ShaderNotify& shader_notify_);

        CLASS_NON_COPYABLE(VulkanGraphics);
        CLASS_NON_MOVEABLE(VulkanGraphics);
        void clean(const CleanValue& cleanValue) override;
        void dispatchCompute(const IComputeInstance& instance) override;
        auto uploadModel(const graphics::IMeshData& instance) -> MeshId override;
        auto uploadTexture(const ::resource::image::ITexture& texture) -> TextureId override;
        auto uploadTexture(ktxTexture* ktxTexture) -> TextureId override;
        void draw(const graphics::IMeshInstance& instance) override;
        auto getDrawImage() -> unsigned long long override;
        auto addShader(std::span<const u32> data, ShaderType type) -> u64 override {
            return pipeline_cache.addShader(data, type);
        };
        ~VulkanGraphics() override;

        auto AccelerateDisplay(const frame::FramebufferConfig& config, u32 pixel_stride)
            -> std::optional<FramebufferTextureInfo>;
        void TickFrame();

    private:
        template <typename Func>
        void PrepareDraw(Func&&);
        void FlushWork();
        void UpdateDynamicStates();
        void UpdatePrimitiveRestartEnable();
        void UpdateRasterizerDiscardEnable();
        void UpdateDepthBiasEnable();
        void UpdateCullMode();

        void UpdateVertexInput();
        void UpdateDepthCompareOp();
        void UpdateFrontFace();
        void UpdateStencilOp();
        void UpdateDepthBoundsTestEnable();
        void UpdateDepthTestEnable();
        void UpdateDepthWriteEnable();
        void UpdateStencilTestEnable();
        void UpdateDepthClampEnable();
        void UpdateBlending();

        void UpdateViewportsState();
        void UpdateScissorsState();
        void UpdateBlendConstants();
        void UpdateDepthBias();
        void UpdateDepthBounds();
        void UpdateStencilFaces();
        void UpdateLineWidth();
        const Device& device;
        MemoryAllocator& memory_allocator;
        scheduler::Scheduler& scheduler;
        Sampler sampler;
        StagingBufferPool staging_pool;
        resource::DescriptorPool descriptor_pool;
        GuestDescriptorQueue guest_descriptor_queue;
        ComputePassDescriptorQueue compute_pass_descriptor_queue;
        RenderPassCache render_pass_cache;
        core::frontend::BaseWindow* emu_window;
        TextureCacheRuntime texture_cache_runtime;
        TextureCache texture_cache;
        BufferCacheRuntime buffer_cache_runtime;
        BufferCache buffer_cache;
        PipelineCache pipeline_cache;
        Event wfi_event;

        u32 draw_counter = 0;
        ModelId current_modelId;
        common::SlotVector<ModelResource> modelResource;
        common::SlotVector<
            boost::container::static_vector<vk::VertexInputAttributeDescription2EXT, 32>>
            vertex_attributes;
        common::SlotVector<
            boost::container::static_vector<vk::VertexInputBindingDescription2EXT, 32>>
            vertex_bindings;

        std::unordered_map<VkImageView, unsigned long long> imgui_textures;
        DynamicPipelineState last_pipeline_state;
        DynamicPipelineState current_pipeline_state;
        PrimitiveTopology current_primitive_topology;

        bool is_begin_frame{true};
};

}  // namespace render::vulkan