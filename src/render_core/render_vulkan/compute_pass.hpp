#pragma once
#include "vulkan_common/vulkan_wrapper.hpp"
#include "descriptor_pool.hpp"
#include "update_descriptor.hpp"
#include <optional>
#include "texture/types.hpp"
#include "vulkan_common/memory_allocator.hpp"

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
}  // namespace render::vulkan