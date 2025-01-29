#pragma once
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include "render_core/render_vulkan/descriptor_pool.hpp"
#include "render_core/render_vulkan/update_descriptor.hpp"
#include <optional>
#include "render_core/texture/types.hpp"
#include "render_core/vulkan_common/memory_allocator.hpp"
#include "render_core/fixed_pipeline_state.h"
namespace render::vulkan {

class Device;
class TextureImage;
class StagingBufferPool;
struct StagingBufferRef;
namespace scheduler {
class Scheduler;
}

class ComputePass {
    public:
        explicit ComputePass(const Device& device, resource::DescriptorPool& descriptor_pool,
                             utils::Span<vk::DescriptorSetLayoutBinding> bindings,
                             utils::Span<vk::DescriptorUpdateTemplateEntry> templates,
                             const resource::DescriptorBankInfo& bank_info,
                             utils::Span<vk::PushConstantRange> push_constants,
                             std::span<const u32> code,
                             std::optional<u32> optional_subgroup_size = std::nullopt);
        ~ComputePass();

    protected:
        const Device& device_;
        DescriptorUpdateTemplate descriptor_template;
        PipelineLayout layout;
        Pipeline pipeline;
        DescriptorSetLayout descriptor_set_layout;
        resource::DescriptorAllocator descriptor_allocator;

    private:
        ShaderModule module;
};

class ASTCDecoderPass final : public ComputePass {
    public:
        explicit ASTCDecoderPass(const Device& device_, scheduler::Scheduler& scheduler_,
                                 resource::DescriptorPool& descriptor_pool_,
                                 StagingBufferPool& staging_buffer_pool_,
                                 ComputePassDescriptorQueue& compute_pass_descriptor_queue_,
                                 MemoryAllocator& memory_allocator_);
        ~ASTCDecoderPass();

        void Assemble(TextureImage& image, const StagingBufferRef& map,
                      std::span<const texture::SwizzleParameters> swizzles);

    private:
        scheduler::Scheduler& scheduler;
        StagingBufferPool& staging_buffer_pool;
        ComputePassDescriptorQueue& compute_pass_descriptor_queue;
        MemoryAllocator& memory_allocator;
};

class MSAACopyPass final : public ComputePass {
    public:
        explicit MSAACopyPass(const Device& device_, scheduler::Scheduler& scheduler_,
                              resource::DescriptorPool& descriptor_pool_,
                              StagingBufferPool& staging_buffer_pool_,
                              ComputePassDescriptorQueue& compute_pass_descriptor_queue_);
        ~MSAACopyPass() = default;

        void CopyImage(TextureImage& dst_image, TextureImage& src_image,
                       std::span<const render::texture::ImageCopy> copies, bool msaa_to_non_msaa);

    private:
        scheduler::Scheduler& scheduler;
        StagingBufferPool& staging_buffer_pool;
        ComputePassDescriptorQueue& compute_pass_descriptor_queue;
        std::array<ShaderModule, 2> modules;
        std::array<Pipeline, 2> pipelines;
};

class QuadIndexedPass final : public ComputePass {
    public:
        explicit QuadIndexedPass(const Device& device_, scheduler::Scheduler& scheduler_,
                                 resource::DescriptorPool& descriptor_pool_,
                                 StagingBufferPool& staging_buffer_pool_,
                                 ComputePassDescriptorQueue& compute_pass_descriptor_queue_);
        ~QuadIndexedPass();

        auto Assemble(IndexFormat index_format, u32 num_vertices, u32 base_vertex,
                      vk::Buffer src_buffer, u32 src_offset, bool is_strip)
            -> std::pair<vk::Buffer, vk::DeviceSize>;

    private:
        scheduler::Scheduler& scheduler;
        StagingBufferPool& staging_buffer_pool;
        ComputePassDescriptorQueue& compute_pass_descriptor_queue;
};

class Uint8Pass final : public ComputePass {
    public:
        explicit Uint8Pass(const Device& device_, scheduler::Scheduler& scheduler_,
                           resource::DescriptorPool& descriptor_pool_,
                           StagingBufferPool& staging_buffer_pool_,
                           ComputePassDescriptorQueue& compute_pass_descriptor_queue_);
        ~Uint8Pass();

        /// Assemble uint8 indices into an uint16 index buffer
        /// Returns a pair with the staging buffer, and the offset where the assembled data is
        auto Assemble(u32 num_vertices, vk::Buffer src_buffer, u32 src_offset)
            -> std::pair<vk::Buffer, vk::DeviceSize>;

    private:
        scheduler::Scheduler& scheduler;
        StagingBufferPool& staging_buffer_pool;
        ComputePassDescriptorQueue& compute_pass_descriptor_queue;
};

}  // namespace render::vulkan