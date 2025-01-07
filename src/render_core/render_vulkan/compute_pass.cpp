#include "vulkan_common/device.hpp"
#include "render_vulkan/scheduler.hpp"
#include "render_vulkan/descriptor_pool.hpp"
#include "update_descriptor.hpp"
#include "compute_pass.hpp"
#include "texture_cache.hpp"
#include "common/div_ceil.hpp"
#include "common/vector_math.h"
#include "render_core/host_shaders/convert_msaa_to_non_msaa_comp_spv.h"
#include "render_core/host_shaders/convert_non_msaa_to_msaa_comp_spv.h"
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
        descriptor_template =
            DescriptorUpdateTemplate{device_.getLogical().createDescriptorUpdateTemplate(
                                         vk::DescriptorUpdateTemplateCreateInfo{
                                             {},
                                             templates,
                                             vk::DescriptorUpdateTemplateType::eDescriptorSet,
                                             *descriptor_set_layout,
                                             vk::PipelineBindPoint::eGraphics,
                                             *layout}),
                                     device_.getLogical()};
    }
    descriptor_allocator = descriptor_pool.allocator(*descriptor_set_layout, bank_info);
    if (code.empty()) {
        return;
    }
    module = ShaderModule{device_.getLogical().createShaderModule(
                              vk::ShaderModuleCreateInfo{{}, code.size_bytes(), code.data()}),
                          device_.getLogical()};

    // device.SaveShader(code);

    vk::PipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroup_size_ci{
        optional_subgroup_size ? *optional_subgroup_size : 32U};

    bool use_setup_size = device_.isExtSubgroupSizeControlSupported() && optional_subgroup_size;
    pipeline = Pipeline{
        device_.getLogical()
            .createComputePipeline(
                vk::PipelineCache{},
                vk::ComputePipelineCreateInfo{
                    {},
                    vk::PipelineShaderStageCreateInfo{{},
                                                      vk::ShaderStageFlagBits::eCompute,
                                                      *module,
                                                      "main",
                                                      nullptr,
                                                      use_setup_size ? &subgroup_size_ci : nullptr},
                    *layout,
                    nullptr,
                    0})
            .value,
        device_.getLogical()};
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
        modules[i] =
            ShaderModule{device_.getLogical().createShaderModule(vk::ShaderModuleCreateInfo{

                             {},
                             static_cast<u32>(code.size_bytes()),
                             code.data(),
                         }),
                         device_.getLogical()};

        pipelines[i] = Pipeline{
            device_.getLogical()
                .createComputePipeline(
                    vk::PipelineCache{},
                    vk::ComputePipelineCreateInfo{

                        {},
                        vk::PipelineShaderStageCreateInfo{
                            {}, vk::ShaderStageFlagBits::eCompute, *modules[i], "main", nullptr},
                        *layout,
                        nullptr,
                        0,
                    })
                .value,
            device_.getLogical()};
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

        const common::Vec3<u32> num_dispatches = {
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

}  // namespace render::vulkan