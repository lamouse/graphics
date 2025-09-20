#include "vulkan_common/device.hpp"
#include "render_vulkan/scheduler.hpp"
#include "render_vulkan/descriptor_pool.hpp"
#include "update_descriptor.hpp"
#include "compute_pass.hpp"
#include "texture_cache.hpp"
#include "common/div_ceil.hpp"
#include "render_core/host_shaders/convert_msaa_to_non_msaa_comp_spv.h"
#include "render_core/host_shaders/convert_non_msaa_to_msaa_comp_spv.h"
#include "render_core/host_shaders/vulkan_quad_indexed_comp_spv.h"
#include "render_core/host_shaders/astc_decoder_comp_spv.h"
#include "render_core/host_shaders/vulkan_uint8_comp_spv.h"
#include "texture/accelerated_swizzle.h"
#include "staging_buffer_pool.hpp"
#include <glm/glm.hpp>

#if defined(MemoryBarrier)
#undef MemoryBarrier
#endif
namespace render::vulkan {
namespace {
constexpr u32 ASTC_BINDING_INPUT_BUFFER = 0;
constexpr u32 ASTC_BINDING_OUTPUT_IMAGE = 1;
constexpr size_t ASTC_NUM_BINDINGS = 2;

template <size_t size>
inline constexpr vk::PushConstantRange COMPUTE_PUSH_CONSTANT_RANGE{
    vk::ShaderStageFlagBits::eCompute,
    0,
    static_cast<u32>(size),
};

constexpr std::array<vk::DescriptorSetLayoutBinding, 2> INPUT_OUTPUT_DESCRIPTOR_SET_BINDINGS{
    {vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageBuffer, 1,
                                    vk::ShaderStageFlagBits::eCompute},
     vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eStorageBuffer, 1,
                                    vk::ShaderStageFlagBits::eCompute}}};

constexpr std::array<vk::DescriptorSetLayoutBinding, 3> QUERIES_SCAN_DESCRIPTOR_SET_BINDINGS{{
    vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageBuffer, 1,
                                   vk::ShaderStageFlagBits::eCompute},
    vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eStorageBuffer, 1,
                                   vk::ShaderStageFlagBits::eCompute},
    vk::DescriptorSetLayoutBinding{2, vk::DescriptorType::eStorageBuffer, 1,
                                   vk::ShaderStageFlagBits::eCompute},
}};

constexpr resource::DescriptorBankInfo INPUT_OUTPUT_BANK_INFO{
    .uniform_buffers_ = 0,
    .storage_buffers_ = 2,
    .texture_buffers_ = 0,
    .image_buffers_ = 0,
    .textures_ = 0,
    .images_ = 0,
    .score_ = 2,
};

constexpr resource::DescriptorBankInfo QUERIES_SCAN_BANK_INFO{
    .uniform_buffers_ = 0,
    .storage_buffers_ = 3,
    .texture_buffers_ = 0,
    .image_buffers_ = 0,
    .textures_ = 0,
    .images_ = 0,
    .score_ = 3,
};

constexpr std::array<vk::DescriptorSetLayoutBinding, ASTC_NUM_BINDINGS>
    ASTC_DESCRIPTOR_SET_BINDINGS{{
        vk::DescriptorSetLayoutBinding{ASTC_BINDING_INPUT_BUFFER,
                                       vk::DescriptorType::eStorageBuffer, 1,
                                       vk::ShaderStageFlagBits::eCompute},
        vk::DescriptorSetLayoutBinding{ASTC_BINDING_OUTPUT_IMAGE, vk::DescriptorType::eStorageImage,
                                       1, vk::ShaderStageFlagBits::eCompute},
    }};

constexpr resource::DescriptorBankInfo ASTC_BANK_INFO{
    .uniform_buffers_ = 0,
    .storage_buffers_ = 1,
    .texture_buffers_ = 0,
    .image_buffers_ = 0,
    .textures_ = 0,
    .images_ = 1,
    .score_ = 2,
};

constexpr std::array<vk::DescriptorSetLayoutBinding, ASTC_NUM_BINDINGS>
    MSAA_DESCRIPTOR_SET_BINDINGS{
        {vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageImage, 1,
                                        vk::ShaderStageFlagBits::eCompute},
         vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eStorageImage, 1,
                                        vk::ShaderStageFlagBits::eCompute}}};

constexpr resource::DescriptorBankInfo MSAA_BANK_INFO{
    .uniform_buffers_ = 0,
    .storage_buffers_ = 0,
    .texture_buffers_ = 0,
    .image_buffers_ = 0,
    .textures_ = 0,
    .images_ = 2,
    .score_ = 2,
};

constexpr vk::DescriptorUpdateTemplateEntry INPUT_OUTPUT_DESCRIPTOR_UPDATE_TEMPLATE{
    0, 0, 2, vk::DescriptorType::eStorageBuffer, 0, sizeof(DescriptorUpdateEntry)};

constexpr vk::DescriptorUpdateTemplateEntry QUERIES_SCAN_DESCRIPTOR_UPDATE_TEMPLATE{
    0, 0, 3, vk::DescriptorType::eStorageBuffer, 0, sizeof(DescriptorUpdateEntry),
};

constexpr vk::DescriptorUpdateTemplateEntry MSAA_DESCRIPTOR_UPDATE_TEMPLATE{
    0, 0, 2, vk::DescriptorType::eStorageImage, 0, sizeof(DescriptorUpdateEntry),
};

constexpr std::array<vk::DescriptorUpdateTemplateEntry, ASTC_NUM_BINDINGS>
    ASTC_PASS_DESCRIPTOR_UPDATE_TEMPLATE_ENTRY{{
        vk::DescriptorUpdateTemplateEntry{
            ASTC_BINDING_INPUT_BUFFER,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            ASTC_BINDING_INPUT_BUFFER * sizeof(DescriptorUpdateEntry),
            sizeof(DescriptorUpdateEntry),
        },
        vk::DescriptorUpdateTemplateEntry{
            ASTC_BINDING_OUTPUT_IMAGE,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            ASTC_BINDING_OUTPUT_IMAGE * sizeof(DescriptorUpdateEntry),
            sizeof(DescriptorUpdateEntry),
        },
    }};

struct AstcPushConstants {
        std::array<u32, 2> blocks_dims;
        u32 layer_stride;
        u32 block_size;
        u32 x_shift;
        u32 block_height;
        u32 block_height_mask;
};

struct QueriesPrefixScanPushConstants {
        u32 min_accumulation_base;
        u32 max_accumulation_base;
        u32 accumulation_limit;
        u32 buffer_offset;
};

}  // namespace

ComputePass::ComputePass(const Device& device, resource::DescriptorPool& descriptor_pool,
                         utils::Span<vk::DescriptorSetLayoutBinding> bindings,
                         utils::Span<vk::DescriptorUpdateTemplateEntry> templates,
                         const resource::DescriptorBankInfo& bank_info,
                         utils::Span<vk::PushConstantRange> push_constants,
                         std::span<const u32> code, std::optional<u32> optional_subgroup_size)
    : device_(device) {
    descriptor_set_layout = device_.logical().createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo{{}, bindings});

    layout = device_.logical().createPipelineLayout(
        vk::PipelineLayoutCreateInfo{{},
                                     1,
                                     descriptor_set_layout.address(),
                                     static_cast<uint32_t>(push_constants.size()),
                                     push_constants.data()});

    if (!templates.empty()) {
        descriptor_template = device_.logical().createDescriptorUpdateTemplate(
            vk::DescriptorUpdateTemplateCreateInfo{{},
                                                   templates,
                                                   vk::DescriptorUpdateTemplateType::eDescriptorSet,
                                                   *descriptor_set_layout,
                                                   vk::PipelineBindPoint::eGraphics,
                                                   *layout});
    }
    descriptor_allocator = descriptor_pool.allocator(*descriptor_set_layout, bank_info);
    if (code.empty()) {
        return;
    }
    module = device_.logical().createShaderModel(
        vk::ShaderModuleCreateInfo{{}, code.size_bytes(), code.data()});

    // device.SaveShader(code);

    vk::PipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroup_size_ci{
        optional_subgroup_size ? *optional_subgroup_size : 32U};

    bool use_setup_size = device_.isExtSubgroupSizeControlSupported() && optional_subgroup_size;
    pipeline = device_.logical().createPipeline(vk::ComputePipelineCreateInfo{
        {},
        vk::PipelineShaderStageCreateInfo{{},
                                          vk::ShaderStageFlagBits::eCompute,
                                          *module,
                                          "main",
                                          nullptr,
                                          use_setup_size ? &subgroup_size_ci : nullptr},
        *layout,
        nullptr,
        0});
}
ComputePass::~ComputePass() = default;

MSAACopyPass::MSAACopyPass(const Device& device_, scheduler::Scheduler& scheduler_,
                           resource::DescriptorPool& descriptor_pool_,
                           StagingBufferPool& staging_buffer_pool_,
                           ComputePassDescriptorQueue& compute_pass_descriptor_queue_)
    : ComputePass(device_, descriptor_pool_, MSAA_DESCRIPTOR_SET_BINDINGS,
                  MSAA_DESCRIPTOR_UPDATE_TEMPLATE, MSAA_BANK_INFO, {},
                  CONVERT_NON_MSAA_TO_MSAA_COMP_SPV),
      scheduler{scheduler_},
      staging_buffer_pool{staging_buffer_pool_},
      compute_pass_descriptor_queue{compute_pass_descriptor_queue_} {
    const auto make_msaa_pipeline = [this, &device_](size_t i, std::span<const u32> code) {
        modules[i] = device_.logical().createShaderModel(vk::ShaderModuleCreateInfo{

            {},
            static_cast<u32>(code.size_bytes()),
            code.data(),
        });

        pipelines[i] = device_.logical().createPipeline(vk::ComputePipelineCreateInfo{

            {},
            vk::PipelineShaderStageCreateInfo{
                {}, vk::ShaderStageFlagBits::eCompute, *modules[i], "main", nullptr},
            *layout,
            nullptr,
            0,
        });
    };
    make_msaa_pipeline(0, CONVERT_NON_MSAA_TO_MSAA_COMP_SPV);
    make_msaa_pipeline(1, CONVERT_MSAA_TO_NON_MSAA_COMP_SPV);
}

void MSAACopyPass::CopyImage(TextureImage& dst_image, TextureImage& src_image,
                             std::span<const render::texture::ImageCopy> copies,
                             bool msaa_to_non_msaa) {
    const vk::Pipeline msaa_pipeline = *pipelines[msaa_to_non_msaa ? 1 : 0];
    scheduler.requestOutsideRenderPassOperationContext();
    for (const render::texture::ImageCopy& copy : copies) {
        assert(copy.src_subresource.base_layer == 0);
        assert(copy.src_subresource.num_layers == 1);
        assert(copy.dst_subresource.base_layer == 0);
        assert(copy.dst_subresource.num_layers == 1);

        compute_pass_descriptor_queue.Acquire();
        compute_pass_descriptor_queue.AddImage(
            src_image.StorageImageView(copy.src_subresource.base_level));
        compute_pass_descriptor_queue.AddImage(
            dst_image.StorageImageView(copy.dst_subresource.base_level));
        const void* const descriptor_data{compute_pass_descriptor_queue.UpdateData()};

        const glm::uvec3 num_dispatches = {
            common::DivCeil(copy.extent.width, 8U),
            common::DivCeil(copy.extent.height, 8U),
            copy.extent.depth,
        };

        scheduler.record([this, dst = dst_image.Handle(), msaa_pipeline, num_dispatches,
                          descriptor_data](vk::CommandBuffer cmdbuf) {
            const vk::DescriptorSet set = descriptor_allocator.commit();
            device_.getLogical().updateDescriptorSetWithTemplate(set, *descriptor_template,
                                                                 descriptor_data);
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, msaa_pipeline);
            cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *layout, 0, set, {});
            cmdbuf.dispatch(num_dispatches.x, num_dispatches.y, num_dispatches.z);
            const vk::ImageMemoryBarrier write_barrier{
                vk::AccessFlagBits::eShaderWrite,
                vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                dst,
                vk::ImageSubresourceRange{
                    vk::ImageAspectFlagBits::eColor,
                    VK_REMAINING_MIP_LEVELS,
                    0,
                    VK_REMAINING_ARRAY_LAYERS,
                },
            };
            cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                   vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                                   write_barrier);
        });
    }
}

ASTCDecoderPass::ASTCDecoderPass(const Device& device_, scheduler::Scheduler& scheduler_,
                                 resource::DescriptorPool& descriptor_pool_,
                                 StagingBufferPool& staging_buffer_pool_,
                                 ComputePassDescriptorQueue& compute_pass_descriptor_queue_,
                                 MemoryAllocator& memory_allocator_)
    : ComputePass(device_, descriptor_pool_, ASTC_DESCRIPTOR_SET_BINDINGS,
                  ASTC_PASS_DESCRIPTOR_UPDATE_TEMPLATE_ENTRY, ASTC_BANK_INFO,
                  COMPUTE_PUSH_CONSTANT_RANGE<sizeof(AstcPushConstants)>, ASTC_DECODER_COMP_SPV),
      scheduler{scheduler_},
      staging_buffer_pool{staging_buffer_pool_},
      compute_pass_descriptor_queue{compute_pass_descriptor_queue_},
      memory_allocator{memory_allocator_} {}

void ASTCDecoderPass::Assemble(TextureImage& image, const StagingBufferRef& map,
                               std::span<const texture::SwizzleParameters> swizzles) {
    using namespace texture::Accelerated;
    const std::array<u32, 2> block_dims{
        surface::DefaultBlockWidth(image.info.format),
        surface::DefaultBlockHeight(image.info.format),
    };
    scheduler.requestOutsideRenderPassOperationContext();
    const vk::Pipeline vk_pipeline = *pipeline;
    const vk::ImageAspectFlags aspect_mask = image.AspectMask();
    const vk::Image vk_image = image.Handle();
    const bool is_initialized = image.ExchangeInitialization();
    scheduler.record(
        [vk_pipeline, vk_image, aspect_mask, is_initialized](vk::CommandBuffer cmdbuf) {
            const vk::ImageMemoryBarrier image_barrier{
                static_cast<vk::AccessFlags>(is_initialized ? VK_ACCESS_SHADER_WRITE_BIT
                                                            : VK_ACCESS_NONE),
                vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
                is_initialized ? vk::ImageLayout::eGeneral : vk::ImageLayout::eUndefined,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                vk_image,
                vk::ImageSubresourceRange{
                    aspect_mask,
                    0,
                    VK_REMAINING_MIP_LEVELS,
                    0,
                    VK_REMAINING_ARRAY_LAYERS,
                },
            };
            cmdbuf.pipelineBarrier(is_initialized ? vk::PipelineStageFlagBits::eAllCommands
                                                  : vk::PipelineStageFlagBits::eTopOfPipe,
                                   vk::PipelineStageFlagBits::eComputeShader, {}, {}, {},
                                   image_barrier);
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, vk_pipeline);
        });
    for (const texture::SwizzleParameters& swizzle : swizzles) {
        const size_t input_offset = swizzle.buffer_offset + map.offset;
        const u32 num_dispatches_x = common::DivCeil(swizzle.num_tiles.width, 8U);
        const u32 num_dispatches_y = common::DivCeil(swizzle.num_tiles.height, 8U);
        const u32 num_dispatches_z = image.info.resources.layers;

        compute_pass_descriptor_queue.Acquire();
        compute_pass_descriptor_queue.AddBuffer(map.buffer, input_offset,
                                                image.guest_size_bytes - swizzle.buffer_offset);
        compute_pass_descriptor_queue.AddImage(image.StorageImageView(swizzle.level));
        const void* const descriptor_data{compute_pass_descriptor_queue.UpdateData()};

        // To unswizzle the ASTC data
        const auto params = MakeBlockLinearSwizzle2DParams(swizzle, image.info);
        assert(params.origin == (std::array<u32, 3>{0, 0, 0}));
        assert(params.destination == (std::array<s32, 3>{0, 0, 0}));
        assert(params.bytes_per_block_log2 == 4);
        scheduler.record([this, num_dispatches_x, num_dispatches_y, num_dispatches_z, block_dims,
                          params, descriptor_data](vk::CommandBuffer cmdbuf) {
            const AstcPushConstants uniforms{
                .blocks_dims = block_dims,
                .layer_stride = params.layer_stride,
                .block_size = params.block_size,
                .x_shift = params.x_shift,
                .block_height = params.block_height,
                .block_height_mask = params.block_height_mask,
            };
            const vk::DescriptorSet set = descriptor_allocator.commit();
            device_.getLogical().updateDescriptorSetWithTemplate(set, *descriptor_template,
                                                                 descriptor_data);
            cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *layout, 0, set, {});
            cmdbuf.pushConstants(*layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(uniforms),
                                 &uniforms);
            cmdbuf.dispatch(num_dispatches_x, num_dispatches_y, num_dispatches_z);
        });
    }
    scheduler.record([vk_image, aspect_mask](vk::CommandBuffer cmdbuf) {
        const vk::ImageMemoryBarrier image_barrier{
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            vk_image,
            vk::ImageSubresourceRange{
                aspect_mask,
                0,
                VK_REMAINING_MIP_LEVELS,
                0,
                VK_REMAINING_ARRAY_LAYERS,
            },
        };
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                               vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, image_barrier);
    });
    scheduler.finish();
}

ASTCDecoderPass::~ASTCDecoderPass() = default;

QuadIndexedPass::QuadIndexedPass(const Device& device_, scheduler::Scheduler& scheduler_,
                                 resource::DescriptorPool& descriptor_pool_,
                                 StagingBufferPool& staging_buffer_pool_,
                                 ComputePassDescriptorQueue& compute_pass_descriptor_queue_)
    : ComputePass(device_, descriptor_pool_, INPUT_OUTPUT_DESCRIPTOR_SET_BINDINGS,
                  INPUT_OUTPUT_DESCRIPTOR_UPDATE_TEMPLATE, INPUT_OUTPUT_BANK_INFO,
                  COMPUTE_PUSH_CONSTANT_RANGE<sizeof(u32) * 3>, VULKAN_QUAD_INDEXED_COMP_SPV),
      scheduler{scheduler_},
      staging_buffer_pool{staging_buffer_pool_},
      compute_pass_descriptor_queue{compute_pass_descriptor_queue_} {}

QuadIndexedPass::~QuadIndexedPass() = default;

auto QuadIndexedPass::Assemble(IndexFormat index_format, u32 num_vertices, u32 base_vertex,
                               vk::Buffer src_buffer, u32 src_offset, bool is_strip)
    -> std::pair<vk::Buffer, vk::DeviceSize> {
    const u32 index_shift = [index_format] {
        switch (index_format) {
            case IndexFormat::UnsignedByte:
                return 0;
            case IndexFormat::UnsignedShort:
                return 1;
            case IndexFormat::UnsignedInt:
                return 2;
        }
        assert(false);
        return 2;
    }();
    const u32 input_size = num_vertices << index_shift;
    const u32 num_tri_vertices = (is_strip ? (num_vertices - 2) / 2 : num_vertices / 4) * 6;

    const std::size_t staging_size = num_tri_vertices * sizeof(u32);
    const auto staging = staging_buffer_pool.Request(staging_size, MemoryUsage::DeviceLocal);

    compute_pass_descriptor_queue.Acquire();
    compute_pass_descriptor_queue.AddBuffer(src_buffer, src_offset, input_size);
    compute_pass_descriptor_queue.AddBuffer(staging.buffer, staging.offset, staging_size);
    const void* const descriptor_data{compute_pass_descriptor_queue.UpdateData()};

    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([this, descriptor_data, num_tri_vertices, base_vertex, index_shift,
                      is_strip](vk::CommandBuffer cmdbuf) {
        static constexpr u32 DISPATCH_SIZE = 1024;

        static constexpr vk::MemoryBarrier WRITE_BARRIER{vk::AccessFlagBits::eShaderWrite,
                                                         vk::AccessFlagBits::eIndexRead};
        const std::array<u32, 3> push_constants{base_vertex, index_shift, is_strip ? 1u : 0u};
        const vk::DescriptorSet set = descriptor_allocator.commit();
        device_.logical().UpdateDescriptorSet(set, *descriptor_template, descriptor_data);
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *layout, 0, set, {});
        cmdbuf.pushConstants(*layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(push_constants),
                             &push_constants);
        cmdbuf.dispatch(common::DivCeil(num_tri_vertices, DISPATCH_SIZE), 1, 1);
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                               vk::PipelineStageFlagBits::eVertexInput, {}, WRITE_BARRIER, {}, {});
    });
    return {staging.buffer, staging.offset};
}

Uint8Pass::Uint8Pass(const Device& device_, scheduler::Scheduler& scheduler_,
                     resource::DescriptorPool& descriptor_pool,
                     StagingBufferPool& staging_buffer_pool_,
                     ComputePassDescriptorQueue& compute_pass_descriptor_queue_)
    : ComputePass(device_, descriptor_pool, INPUT_OUTPUT_DESCRIPTOR_SET_BINDINGS,
                  INPUT_OUTPUT_DESCRIPTOR_UPDATE_TEMPLATE, INPUT_OUTPUT_BANK_INFO, {},
                  VULKAN_UINT8_COMP_SPV),
      scheduler{scheduler_},
      staging_buffer_pool{staging_buffer_pool_},
      compute_pass_descriptor_queue{compute_pass_descriptor_queue_} {}

Uint8Pass::~Uint8Pass() = default;
auto Uint8Pass::Assemble(u32 num_vertices, vk::Buffer src_buffer, u32 src_offset)
    -> std::pair<vk::Buffer, vk::DeviceSize> {
    const u32 staging_size = static_cast<u32>(num_vertices * sizeof(u16));
    const auto staging = staging_buffer_pool.Request(staging_size, MemoryUsage::DeviceLocal);

    compute_pass_descriptor_queue.Acquire();
    compute_pass_descriptor_queue.AddBuffer(src_buffer, src_offset, num_vertices);
    compute_pass_descriptor_queue.AddBuffer(staging.buffer, staging.offset, staging_size);
    const void* const descriptor_data{compute_pass_descriptor_queue.UpdateData()};

    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([this, descriptor_data, num_vertices](vk::CommandBuffer cmdbuf) {
        static constexpr u32 DISPATCH_SIZE = 1024;
        static constexpr vk::MemoryBarrier WRITE_BARRIER{vk::AccessFlagBits::eShaderWrite,
                                                         vk::AccessFlagBits::eVertexAttributeRead};
        const vk::DescriptorSet set = descriptor_allocator.commit();
        device_.logical().UpdateDescriptorSet(set, *descriptor_template, descriptor_data);
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *layout, 0, set, {});
        cmdbuf.dispatch(common::DivCeil(num_vertices, DISPATCH_SIZE), 1, 1);
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                               vk::PipelineStageFlagBits::eVertexInput, {}, WRITE_BARRIER, {}, {});
    });
    return {staging.buffer, staging.offset};
}
}  // namespace render::vulkan