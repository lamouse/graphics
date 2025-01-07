// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "texture_cache.hpp"

#include "common/settings.hpp"
#include "render_core/host_shaders/blit_color_float_frag_spv.h"
#include "render_core/host_shaders/convert_abgr8_to_d24s8_frag_spv.h"
#include "render_core/host_shaders/convert_abgr8_to_d32f_frag_spv.h"
#include "render_core/host_shaders/convert_d24s8_to_abgr8_frag_spv.h"
#include "render_core/host_shaders/convert_d32f_to_abgr8_frag_spv.h"
#include "render_core/host_shaders/convert_depth_to_float_frag_spv.h"
#include "render_core/host_shaders/convert_float_to_depth_frag_spv.h"
#include "render_core/host_shaders/convert_s8d24_to_abgr8_frag_spv.h"
#include "render_core/host_shaders/full_screen_triangle_vert_spv.h"
#include "render_core/host_shaders/vulkan_blit_depth_stencil_frag_spv.h"
#include "render_core/host_shaders/vulkan_color_clear_frag_spv.h"
#include "render_core/host_shaders/vulkan_color_clear_vert_spv.h"
#include "render_core/host_shaders/vulkan_depthstencil_clear_frag_spv.h"
#include "blit_image.hpp"
#include "scheduler.hpp"
#include "vk_shader_util.hpp"
#include "update_descriptor.hpp"
#include "vulkan_common/device.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"

namespace render::vulkan {

using texture::ImageViewType;

namespace {
struct PushConstants {
        std::array<float, 2> tex_scale;
        std::array<float, 2> tex_offset;
};

template <u32 binding>
inline constexpr vk::DescriptorSetLayoutBinding TEXTURE_DESCRIPTOR_SET_LAYOUT_BINDING{
    binding, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment,
    nullptr,
};
constexpr std::array TWO_TEXTURES_DESCRIPTOR_SET_LAYOUT_BINDINGS{
    TEXTURE_DESCRIPTOR_SET_LAYOUT_BINDING<0>,
    TEXTURE_DESCRIPTOR_SET_LAYOUT_BINDING<1>,
};
const vk::DescriptorSetLayoutCreateInfo ONE_TEXTURE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO{
    {}, TEXTURE_DESCRIPTOR_SET_LAYOUT_BINDING<0>};
template <u32 num_textures>
inline constexpr resource::DescriptorBankInfo TEXTURE_DESCRIPTOR_BANK_INFO{
    .uniform_buffers_ = 0,
    .storage_buffers_ = 0,
    .texture_buffers_ = 0,
    .image_buffers_ = 0,
    .textures_ = num_textures,
    .images_ = 0,
    .score_ = 2,
};
constexpr vk::DescriptorSetLayoutCreateInfo TWO_TEXTURES_DESCRIPTOR_SET_LAYOUT_CREATE_INFO{
    vk::DescriptorSetLayoutCreateFlags{},
    static_cast<uint32_t>(TWO_TEXTURES_DESCRIPTOR_SET_LAYOUT_BINDINGS.size()),
    TWO_TEXTURES_DESCRIPTOR_SET_LAYOUT_BINDINGS.data()};

template <vk::ShaderStageFlagBits stageFlags, size_t size>
inline constexpr vk::PushConstantRange PUSH_CONSTANT_RANGE{
    stageFlags,
    0,
    static_cast<u32>(size),
};
constexpr vk::PipelineVertexInputStateCreateInfo PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO{
    {}, 0, nullptr, 0, nullptr,
};
constexpr vk::PipelineInputAssemblyStateCreateInfo PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO{
    {},
    vk::PrimitiveTopology::eTriangleList,
    VK_FALSE,
};
constexpr vk::PipelineViewportStateCreateInfo PIPELINE_VIEWPORT_STATE_CREATE_INFO{
    {}, 1, nullptr, 1, nullptr,
};
constexpr vk::PipelineRasterizationStateCreateInfo PIPELINE_RASTERIZATION_STATE_CREATE_INFO{

    {},
    VK_FALSE,
    VK_FALSE,
    vk::PolygonMode::eFill,
    vk::CullModeFlagBits::eBack,
    vk::FrontFace::eClockwise,
    VK_FALSE,
    0.0f,
    0.0f,
    0.0f,
    1.0f,
};
constexpr vk::PipelineMultisampleStateCreateInfo PIPELINE_MULTISAMPLE_STATE_CREATE_INFO{

    {}, vk::SampleCountFlagBits::e1, VK_FALSE, 0.0f, nullptr, VK_FALSE, VK_FALSE,
};
constexpr std::array DYNAMIC_STATES{vk::DynamicState::eViewport, vk::DynamicState::eScissor,
                                    vk::DynamicState::eBlendConstants};
constexpr vk::PipelineDynamicStateCreateInfo PIPELINE_DYNAMIC_STATE_CREATE_INFO{
    {},
    static_cast<u32>(DYNAMIC_STATES.size()),
    DYNAMIC_STATES.data(),
};
constexpr vk::PipelineColorBlendStateCreateInfo PIPELINE_COLOR_BLEND_STATE_EMPTY_CREATE_INFO{
    {}, VK_FALSE, vk::LogicOp::eClear, 0, nullptr, {0.0f, 0.0f, 0.0f, 0.0f},
};
constexpr vk::PipelineColorBlendAttachmentState PIPELINE_COLOR_BLEND_ATTACHMENT_STATE{
    VK_FALSE,
    vk::BlendFactor::eZero,
    vk::BlendFactor::eZero,
    vk::BlendOp::eAdd,
    vk::BlendFactor::eZero,
    vk::BlendFactor::eZero,
    vk::BlendOp::eAdd,
    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
constexpr vk::PipelineColorBlendStateCreateInfo PIPELINE_COLOR_BLEND_STATE_GENERIC_CREATE_INFO{
    {},
    VK_FALSE,
    vk::LogicOp::eClear,
    1,
    &PIPELINE_COLOR_BLEND_ATTACHMENT_STATE,
    {0.0f, 0.0f, 0.0f, 0.0f},
};
constexpr vk::PipelineDepthStencilStateCreateInfo PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO{
    {},
    VK_TRUE,
    VK_TRUE,
    vk::CompareOp::eAlways,
    VK_FALSE,
    VK_FALSE,
    vk::StencilOpState{},
    vk::StencilOpState{},
    0.0f,
    0.0f,
};

template <VkFilter filter>
inline constexpr VkSamplerCreateInfo SAMPLER_CREATE_INFO{
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .magFilter = filter,
    .minFilter = filter,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    .mipLodBias = 0.0f,
    .anisotropyEnable = VK_FALSE,
    .maxAnisotropy = 0.0f,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_NEVER,
    .minLod = 0.0f,
    .maxLod = 0.0f,
    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    .unnormalizedCoordinates = VK_TRUE,
};

constexpr vk::PipelineLayoutCreateInfo PipelineLayoutCreateInfo(
    const vk::DescriptorSetLayout* set_layout, utils::Span<vk::PushConstantRange> push_constants) {
    return vk::PipelineLayoutCreateInfo{{},
                                        (set_layout != nullptr ? 1u : 0u),
                                        set_layout,
                                        push_constants.size(),
                                        push_constants.data()};
}

constexpr vk::PipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(
    vk::ShaderStageFlagBits stage, vk::ShaderModule shader) {
    return vk::PipelineShaderStageCreateInfo{
        {}, stage, shader, "main", nullptr,
    };
}

constexpr auto MakeStages(vk::ShaderModule vertex_shader, vk::ShaderModule fragment_shader)
    -> std::array<vk::PipelineShaderStageCreateInfo, 2> {
    return std::array{
        PipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex, vertex_shader),
        PipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment, fragment_shader),
    };
}

void UpdateOneTextureDescriptorSet(const Device& device, VkDescriptorSet descriptor_set,
                                   VkSampler sampler, VkImageView image_view) {
    const vk::DescriptorImageInfo image_info{
        sampler,
        image_view,
        vk::ImageLayout::eGeneral,
    };
    const vk::WriteDescriptorSet write_descriptor_set{

        descriptor_set, 0,       0,       1, vk::DescriptorType::eCombinedImageSampler,
        &image_info,    nullptr, nullptr,
    };
    device.getLogical().updateDescriptorSets(write_descriptor_set, {});
}

void UpdateTwoTexturesDescriptorSet(const Device& device, VkDescriptorSet descriptor_set,
                                    VkSampler sampler, VkImageView image_view_0,
                                    VkImageView image_view_1) {
    const vk::DescriptorImageInfo image_info_0{sampler, image_view_0, vk::ImageLayout::eGeneral};
    const vk::DescriptorImageInfo image_info_1{
        sampler,
        image_view_1,
        vk::ImageLayout::eGeneral,
    };
    const std::array write_descriptor_sets{
        vk::WriteDescriptorSet{

            descriptor_set,
            0,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &image_info_0,
            nullptr,
            nullptr,
        },
        vk::WriteDescriptorSet{
            descriptor_set,
            1,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &image_info_1,
            nullptr,
            nullptr,
        },
    };
    device.getLogical().updateDescriptorSets(write_descriptor_sets, nullptr);
}

void BindBlitState(vk::CommandBuffer cmdbuf, const Region2D& dst_region) {
    const VkOffset2D offset{
        .x = std::min(dst_region.start.x, dst_region.end.x),
        .y = std::min(dst_region.start.y, dst_region.end.y),
    };
    const VkExtent2D extent{
        .width = static_cast<u32>(std::abs(dst_region.end.x - dst_region.start.x)),
        .height = static_cast<u32>(std::abs(dst_region.end.y - dst_region.start.y)),
    };
    const vk::Viewport viewport{
        static_cast<float>(offset.x),
        static_cast<float>(offset.y),
        static_cast<float>(extent.width),
        static_cast<float>(extent.height),
        0.0f,
        1.0f,
    };
    // TODO: Support scissored blits
    const vk::Rect2D scissor{
        offset,
        extent,
    };
    cmdbuf.setViewport(0, viewport);
    cmdbuf.setScissor(0, scissor);
}

void BindBlitState(vk::CommandBuffer cmdbuf, vk::PipelineLayout layout, const Region2D& dst_region,
                   const Region2D& src_region, const Extent3D& src_size = {1, 1, 1}) {
    BindBlitState(cmdbuf, dst_region);
    const float scale_x = static_cast<float>(src_region.end.x - src_region.start.x) /
                          static_cast<float>(src_size.width);
    const float scale_y = static_cast<float>(src_region.end.y - src_region.start.y) /
                          static_cast<float>(src_size.height);
    const PushConstants push_constants{
        .tex_scale = {scale_x, scale_y},
        .tex_offset = {static_cast<float>(src_region.start.x) / static_cast<float>(src_size.width),
                       static_cast<float>(src_region.start.y) /
                           static_cast<float>(src_size.height)},
    };
    cmdbuf.pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0,
                         static_cast<u32>(sizeof(push_constants)), &push_constants);
}

VkExtent2D GetConversionExtent(const TextureImageView& src_image_view) {
    auto resolution = common::settings::get<settings::ResolutionScalingInfo>();
    const bool is_rescaled = src_image_view.IsRescaled();
    u32 width = src_image_view.size.width;
    u32 height = src_image_view.size.height;
    return VkExtent2D{
        .width = is_rescaled ? resolution.ScaleUp(width) : width,
        .height = is_rescaled ? resolution.ScaleUp(height) : height,
    };
}

void TransitionImageLayout(vk::CommandBuffer& cmdbuf, vk::Image image,
                           vk::ImageLayout target_layout,
                           vk::ImageLayout source_layout = vk::ImageLayout::eGeneral) {
    constexpr vk::AccessFlags flags{vk::AccessFlagBits::eColorAttachmentRead |
                                    vk::AccessFlagBits::eColorAttachmentWrite |
                                    vk::AccessFlagBits::eShaderRead};
    const vk::ImageMemoryBarrier barrier{
        flags,
        flags,
        source_layout,
        target_layout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange{
            vk::ImageAspectFlagBits::eColor,
            0,
            1,
            0,
            1,
        },
    };
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                           vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, barrier);
}

void BeginRenderPass(vk::CommandBuffer& cmdbuf, const TextureFramebuffer* framebuffer) {
    const VkRenderPass render_pass = framebuffer->RenderPass();
    const VkFramebuffer framebuffer_handle = framebuffer->Handle();
    const VkExtent2D render_area = framebuffer->RenderArea();
    const vk::RenderPassBeginInfo renderpass_bi{
        render_pass,
        framebuffer_handle,
        vk::Rect2D{
            {},
            render_area,
        },
        0,
        nullptr,
    };
    cmdbuf.beginRenderPass(renderpass_bi, vk::SubpassContents::eInline);
}
}  // Anonymous namespace

BlitImageHelper::BlitImageHelper(const Device& device_, scheduler::Scheduler& scheduler_,
                                 resource::DescriptorPool& descriptor_pool)
    : device{device_},
      scheduler{scheduler_},
      one_texture_set_layout(DescriptorSetLayout{device.getLogical().createDescriptorSetLayout(
                                                     ONE_TEXTURE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO),
                                                 device_.getLogical()}),
      two_textures_set_layout(
          DescriptorSetLayout{device.getLogical().createDescriptorSetLayout(
                                  TWO_TEXTURES_DESCRIPTOR_SET_LAYOUT_CREATE_INFO),
                              device_.getLogical()}),
      one_texture_descriptor_allocator{
          descriptor_pool.allocator(*one_texture_set_layout, TEXTURE_DESCRIPTOR_BANK_INFO<1>)},
      two_textures_descriptor_allocator{
          descriptor_pool.allocator(*two_textures_set_layout, TEXTURE_DESCRIPTOR_BANK_INFO<2>)},
      one_texture_pipeline_layout(PipelineLayout{
          device.getLogical().createPipelineLayout(PipelineLayoutCreateInfo(
              one_texture_set_layout.address(),
              PUSH_CONSTANT_RANGE<vk::ShaderStageFlagBits::eVertex, sizeof(PushConstants)>)),
          device_.getLogical()}),
      two_textures_pipeline_layout(PipelineLayout{
          device.getLogical().createPipelineLayout(PipelineLayoutCreateInfo(
              two_textures_set_layout.address(),
              PUSH_CONSTANT_RANGE<vk::ShaderStageFlagBits::eVertex, sizeof(PushConstants)>)),
          device_.getLogical()}),
      clear_color_pipeline_layout(PipelineLayout{
          device.getLogical().createPipelineLayout(PipelineLayoutCreateInfo(
              nullptr, PUSH_CONSTANT_RANGE<vk::ShaderStageFlagBits::eFragment, sizeof(float) * 4>)),
          device_.getLogical()}),
      full_screen_vert(utils::buildShader(device.getLogical(), FULL_SCREEN_TRIANGLE_VERT_SPV)),
      blit_color_to_color_frag(utils::buildShader(device.getLogical(), BLIT_COLOR_FLOAT_FRAG_SPV)),
      blit_depth_stencil_frag(
          utils::buildShader(device.getLogical(), VULKAN_BLIT_DEPTH_STENCIL_FRAG_SPV)),
      clear_color_vert(utils::buildShader(device.getLogical(), VULKAN_COLOR_CLEAR_VERT_SPV)),
      clear_color_frag(utils::buildShader(device.getLogical(), VULKAN_COLOR_CLEAR_FRAG_SPV)),
      clear_stencil_frag(
          utils::buildShader(device.getLogical(), VULKAN_DEPTHSTENCIL_CLEAR_FRAG_SPV)),
      convert_depth_to_float_frag(
          utils::buildShader(device.getLogical(), CONVERT_DEPTH_TO_FLOAT_FRAG_SPV)),
      convert_float_to_depth_frag(
          utils::buildShader(device.getLogical(), CONVERT_FLOAT_TO_DEPTH_FRAG_SPV)),
      convert_abgr8_to_d24s8_frag(
          utils::buildShader(device.getLogical(), CONVERT_ABGR8_TO_D24S8_FRAG_SPV)),
      convert_abgr8_to_d32f_frag(
          utils::buildShader(device.getLogical(), CONVERT_ABGR8_TO_D32F_FRAG_SPV)),
      convert_d32f_to_abgr8_frag(
          utils::buildShader(device.getLogical(), CONVERT_D32F_TO_ABGR8_FRAG_SPV)),
      convert_d24s8_to_abgr8_frag(
          utils::buildShader(device.getLogical(), CONVERT_D24S8_TO_ABGR8_FRAG_SPV)),
      convert_s8d24_to_abgr8_frag(
          utils::buildShader(device.getLogical(), CONVERT_S8D24_TO_ABGR8_FRAG_SPV)),
      linear_sampler(
          Sampler{device.getLogical().createSampler(SAMPLER_CREATE_INFO<VK_FILTER_LINEAR>),
                  device_.getLogical()}),
      nearest_sampler(
          Sampler{device.getLogical().createSampler(SAMPLER_CREATE_INFO<VK_FILTER_NEAREST>),
                  device_.getLogical()}) {}

BlitImageHelper::~BlitImageHelper() = default;

void BlitImageHelper::BlitColor(const TextureFramebuffer* dst_framebuffer, VkImageView src_view,
                                const Region2D& dst_region, const Region2D& src_region,
                                Operation operation) {
    const bool is_linear = false;
    const BlitImagePipelineKey key{ .operation = operation,
        .renderpass = dst_framebuffer->RenderPass()
                                   };
    const VkPipelineLayout layout = *one_texture_pipeline_layout;
    const VkSampler sampler = is_linear ? *linear_sampler : *nearest_sampler;
    const VkPipeline pipeline = FindOrEmplaceColorPipeline(key);
    scheduler.requestRenderpass(dst_framebuffer);
    scheduler.record([this, dst_region, src_region, pipeline, layout, sampler,
                      src_view](vk::CommandBuffer cmdbuf) {
        // TODO: Barriers
        const vk::DescriptorSet descriptor_set = one_texture_descriptor_allocator.commit();
        UpdateOneTextureDescriptorSet(device, descriptor_set, sampler, src_view);
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, descriptor_set,
                                  nullptr);
        BindBlitState(cmdbuf, layout, dst_region, src_region);
        cmdbuf.draw(3, 1, 0, 0);
    });
    scheduler.invalidateState();
}

void BlitImageHelper::BlitColor(const TextureFramebuffer* dst_framebuffer,
                                VkImageView src_image_view, VkImage src_image,
                                VkSampler src_sampler, const Region2D& dst_region,
                                const Region2D& src_region, const Extent3D& src_size) {
    const BlitImagePipelineKey key{ .operation = Operation::SrcCopy,
        .renderpass = dst_framebuffer->RenderPass(),
                                   };
    const VkPipelineLayout layout = *one_texture_pipeline_layout;
    const VkPipeline pipeline = FindOrEmplaceColorPipeline(key);
    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([this, dst_framebuffer, src_image_view, src_image, src_sampler, dst_region,
                      src_region, src_size, pipeline, layout](vk::CommandBuffer cmdbuf) {
        TransitionImageLayout(cmdbuf, src_image, vk::ImageLayout::eReadOnlyOptimal);
        BeginRenderPass(cmdbuf, dst_framebuffer);
        const vk::DescriptorSet descriptor_set = one_texture_descriptor_allocator.commit();
        UpdateOneTextureDescriptorSet(device, descriptor_set, src_sampler, src_image_view);
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, descriptor_set,
                                  nullptr);
        BindBlitState(cmdbuf, layout, dst_region, src_region, src_size);
        cmdbuf.draw(3, 1, 0, 0);
        cmdbuf.endRenderPass();
    });
}

void BlitImageHelper::BlitDepthStencil(const TextureFramebuffer* dst_framebuffer,
                                       VkImageView src_depth_view, VkImageView src_stencil_view,
                                       const Region2D& dst_region, const Region2D& src_region) {
    if (!device.isExtShaderStencilExportSupported()) {
        return;
    }
    const BlitImagePipelineKey key{
        .renderpass = dst_framebuffer->RenderPass(),
    };
    const VkPipelineLayout layout = *two_textures_pipeline_layout;
    const VkSampler sampler = *nearest_sampler;
    const VkPipeline pipeline = FindOrEmplaceDepthStencilPipeline(key);
    scheduler.requestRenderpass(dst_framebuffer);
    scheduler.record([dst_region, src_region, pipeline, layout, sampler, src_depth_view,
                      src_stencil_view, this](vk::CommandBuffer cmdbuf) {
        // TODO: Barriers
        const vk::DescriptorSet descriptor_set = two_textures_descriptor_allocator.commit();
        UpdateTwoTexturesDescriptorSet(device, descriptor_set, sampler, src_depth_view,
                                       src_stencil_view);
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, descriptor_set,
                                  nullptr);
        BindBlitState(cmdbuf, layout, dst_region, src_region);
        cmdbuf.draw(3, 1, 0, 0);
    });
    scheduler.invalidateState();
}

void BlitImageHelper::ConvertD32ToR32(const TextureFramebuffer* dst_framebuffer,
                                      const TextureImageView& src_image_view) {
    ConvertDepthToColorPipeline(convert_d32_to_r32_pipeline, dst_framebuffer->RenderPass());
    Convert(*convert_d32_to_r32_pipeline, dst_framebuffer, src_image_view);
}

void BlitImageHelper::ConvertR32ToD32(const TextureFramebuffer* dst_framebuffer,
                                      const TextureImageView& src_image_view) {
    ConvertColorToDepthPipeline(convert_r32_to_d32_pipeline, dst_framebuffer->RenderPass());
    Convert(*convert_r32_to_d32_pipeline, dst_framebuffer, src_image_view);
}

void BlitImageHelper::ConvertD16ToR16(const TextureFramebuffer* dst_framebuffer,
                                      const TextureImageView& src_image_view) {
    ConvertDepthToColorPipeline(convert_d16_to_r16_pipeline, dst_framebuffer->RenderPass());
    Convert(*convert_d16_to_r16_pipeline, dst_framebuffer, src_image_view);
}

void BlitImageHelper::ConvertR16ToD16(const TextureFramebuffer* dst_framebuffer,
                                      const TextureImageView& src_image_view) {
    ConvertColorToDepthPipeline(convert_r16_to_d16_pipeline, dst_framebuffer->RenderPass());
    Convert(*convert_r16_to_d16_pipeline, dst_framebuffer, src_image_view);
}

void BlitImageHelper::ConvertABGR8ToD24S8(const TextureFramebuffer* dst_framebuffer,
                                          const TextureImageView& src_image_view) {
    ConvertPipelineDepthTargetEx(convert_abgr8_to_d24s8_pipeline, dst_framebuffer->RenderPass(),
                                 convert_abgr8_to_d24s8_frag);
    Convert(*convert_abgr8_to_d24s8_pipeline, dst_framebuffer, src_image_view);
}

void BlitImageHelper::ConvertABGR8ToD32F(const TextureFramebuffer* dst_framebuffer,
                                         const TextureImageView& src_image_view) {
    ConvertPipelineDepthTargetEx(convert_abgr8_to_d32f_pipeline, dst_framebuffer->RenderPass(),
                                 convert_abgr8_to_d32f_frag);
    Convert(*convert_abgr8_to_d32f_pipeline, dst_framebuffer, src_image_view);
}

void BlitImageHelper::ConvertD32FToABGR8(const TextureFramebuffer* dst_framebuffer,
                                         TextureImageView& src_image_view) {
    ConvertPipelineColorTargetEx(convert_d32f_to_abgr8_pipeline, dst_framebuffer->RenderPass(),
                                 convert_d32f_to_abgr8_frag);
    ConvertDepthStencil(*convert_d32f_to_abgr8_pipeline, dst_framebuffer, src_image_view);
}

void BlitImageHelper::ConvertD24S8ToABGR8(const TextureFramebuffer* dst_framebuffer,
                                          TextureImageView& src_image_view) {
    ConvertPipelineColorTargetEx(convert_d24s8_to_abgr8_pipeline, dst_framebuffer->RenderPass(),
                                 convert_d24s8_to_abgr8_frag);
    ConvertDepthStencil(*convert_d24s8_to_abgr8_pipeline, dst_framebuffer, src_image_view);
}

void BlitImageHelper::ConvertS8D24ToABGR8(const TextureFramebuffer* dst_framebuffer,
                                          TextureImageView& src_image_view) {
    ConvertPipelineColorTargetEx(convert_s8d24_to_abgr8_pipeline, dst_framebuffer->RenderPass(),
                                 convert_s8d24_to_abgr8_frag);
    ConvertDepthStencil(*convert_s8d24_to_abgr8_pipeline, dst_framebuffer, src_image_view);
}

void BlitImageHelper::ClearColor(const TextureFramebuffer* dst_framebuffer, u8 color_mask,
                                 const std::array<f32, 4>& clear_color,
                                 const Region2D& dst_region) {
    const BlitImagePipelineKey key{.renderpass = dst_framebuffer->RenderPass()};
    const VkPipeline pipeline = FindOrEmplaceClearColorPipeline(key);
    const vk::PipelineLayout layout = *clear_color_pipeline_layout;
    scheduler.requestRenderpass(dst_framebuffer);
    scheduler.record(
        [pipeline, layout, color_mask, clear_color, dst_region](vk::CommandBuffer cmdbuf) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
            const std::array blend_color = {
                (color_mask & 0x1) ? 1.0f : 0.0f, (color_mask & 0x2) ? 1.0f : 0.0f,
                (color_mask & 0x4) ? 1.0f : 0.0f, (color_mask & 0x8) ? 1.0f : 0.0f};
            cmdbuf.setBlendConstants(blend_color.data());
            BindBlitState(cmdbuf, dst_region);
            cmdbuf.pushConstants(layout, vk::ShaderStageFlagBits::eFragment, 0,
                                 static_cast<u32>(sizeof(clear_color)), &clear_color);
            cmdbuf.draw(3, 1, 0, 0);
        });
    scheduler.invalidateState();
}

void BlitImageHelper::ClearDepthStencil(const TextureFramebuffer* dst_framebuffer, bool depth_clear,
                                        f32 clear_depth, u8 stencil_mask, u32 stencil_ref,
                                        u32 stencil_compare_mask, const Region2D& dst_region) {
    const BlitDepthStencilPipelineKey key{
        .renderpass = dst_framebuffer->RenderPass(),
        .depth_clear = depth_clear,
        .stencil_mask = stencil_mask,
        .stencil_compare_mask = stencil_compare_mask,
        .stencil_ref = stencil_ref,
    };
    const VkPipeline pipeline = FindOrEmplaceClearStencilPipeline(key);
    const VkPipelineLayout layout = *clear_color_pipeline_layout;
    scheduler.requestRenderpass(dst_framebuffer);
    scheduler.record([pipeline, layout, clear_depth, dst_region](vk::CommandBuffer cmdbuf) {
        constexpr std::array blend_constants{0.0f, 0.0f, 0.0f, 0.0f};
        cmdbuf.setBlendConstants(blend_constants.data());
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        BindBlitState(cmdbuf, dst_region);
        cmdbuf.pushConstants(layout, vk::ShaderStageFlagBits::eFragment, 0,
                             static_cast<u32>(sizeof(clear_depth)), &clear_depth);
        cmdbuf.draw(3, 1, 0, 0);
    });
    scheduler.invalidateState();
}

void BlitImageHelper::Convert(VkPipeline pipeline, const TextureFramebuffer* dst_framebuffer,
                              const TextureImageView& src_image_view) {
    const VkPipelineLayout layout = *one_texture_pipeline_layout;
    const VkImageView src_view = src_image_view.Handle(shader::TextureType::Color2D);
    const VkSampler sampler = *nearest_sampler;
    const VkExtent2D extent = GetConversionExtent(src_image_view);

    scheduler.requestRenderpass(dst_framebuffer);
    scheduler.record([pipeline, layout, sampler, src_view, extent, this](vk::CommandBuffer cmdbuf) {
        const VkOffset2D offset{
            .x = 0,
            .y = 0,
        };
        const vk::Viewport viewport{
            0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height),
            0.0f, 0.0f,
        };
        const vk::Rect2D scissor{
            offset,
            extent,
        };
        const PushConstants push_constants{
            .tex_scale = {viewport.width, viewport.height},
            .tex_offset = {0.0f, 0.0f},
        };
        const vk::DescriptorSet descriptor_set = one_texture_descriptor_allocator.commit();
        UpdateOneTextureDescriptorSet(device, descriptor_set, sampler, src_view);

        // TODO: Barriers
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, static_cast<vk::Pipeline>(pipeline));
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, descriptor_set,
                                  nullptr);
        cmdbuf.setViewport(0, viewport);
        cmdbuf.setScissor(0, scissor);
        cmdbuf.pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_constants),
                             &push_constants);
        cmdbuf.draw(3, 1, 0, 0);
    });
    scheduler.invalidateState();
}

void BlitImageHelper::ConvertDepthStencil(VkPipeline pipeline,
                                          const TextureFramebuffer* dst_framebuffer,
                                          TextureImageView& src_image_view) {
    const VkPipelineLayout layout = *two_textures_pipeline_layout;
    const VkImageView src_depth_view = src_image_view.DepthView();
    const VkImageView src_stencil_view = src_image_view.StencilView();
    const VkSampler sampler = *nearest_sampler;
    const VkExtent2D extent = GetConversionExtent(src_image_view);

    scheduler.requestRenderpass(dst_framebuffer);
    scheduler.record([pipeline, layout, sampler, src_depth_view, src_stencil_view, extent,
                      this](vk::CommandBuffer cmdbuf) {
        const VkOffset2D offset{
            .x = 0,
            .y = 0,
        };
        const vk::Viewport viewport{
            0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height),
            0.0f, 0.0f,
        };
        const vk::Rect2D scissor{
            offset,
            extent,
        };
        const PushConstants push_constants{
            .tex_scale = {viewport.width, viewport.height},
            .tex_offset = {0.0f, 0.0f},
        };
        const vk::DescriptorSet descriptor_set = two_textures_descriptor_allocator.commit();
        UpdateTwoTexturesDescriptorSet(device, descriptor_set, sampler, src_depth_view,
                                       src_stencil_view);
        // TODO: Barriers
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, descriptor_set,
                                  nullptr);
        cmdbuf.setViewport(0, viewport);
        cmdbuf.setScissor(0, scissor);
        cmdbuf.pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_constants),
                             &push_constants);
        cmdbuf.draw(3, 1, 0, 0);
    });
    scheduler.invalidateState();
}

VkPipeline BlitImageHelper::FindOrEmplaceColorPipeline(const BlitImagePipelineKey& key) {
    const auto it = std::ranges::find(blit_color_keys, key);
    if (it != blit_color_keys.end()) {
        return *blit_color_pipelines[std::distance(blit_color_keys.begin(), it)];
    }
    blit_color_keys.push_back(key);

    const std::array stages = MakeStages(*full_screen_vert, *blit_color_to_color_frag);
    const vk::PipelineColorBlendAttachmentState blend_attachment{
        VK_FALSE,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    // TODO: programmable blending
    const vk::PipelineColorBlendStateCreateInfo color_blend_create_info{
        {}, VK_FALSE, vk::LogicOp::eClear, 1, &blend_attachment, {0.0f, 0.0f, 0.0f, 0.0f},
    };
    blit_color_pipelines.push_back(device.logical().createPipeline(
        vk::GraphicsPipelineCreateInfo{{},
                                       stages,
                                       &PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                                       &PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                       nullptr,
                                       &PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                       &PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                       &PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                       nullptr,
                                       &color_blend_create_info,
                                       &PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                       *one_texture_pipeline_layout,
                                       key.renderpass,
                                       0,
                                       VK_NULL_HANDLE,
                                       0}));
    return *blit_color_pipelines.back();
}

VkPipeline BlitImageHelper::FindOrEmplaceDepthStencilPipeline(const BlitImagePipelineKey& key) {
    const auto it = std::ranges::find(blit_depth_stencil_keys, key);
    if (it != blit_depth_stencil_keys.end()) {
        return *blit_depth_stencil_pipelines[std::distance(blit_depth_stencil_keys.begin(), it)];
    }
    blit_depth_stencil_keys.push_back(key);
    const std::array stages = MakeStages(*full_screen_vert, *blit_depth_stencil_frag);
    blit_depth_stencil_pipelines.push_back(device.logical().createPipeline(
        vk::GraphicsPipelineCreateInfo{{},
                                       stages,
                                       &PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                                       &PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                       nullptr,
                                       &PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                       &PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                       &PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                       &PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                       &PIPELINE_COLOR_BLEND_STATE_GENERIC_CREATE_INFO,
                                       &PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                       *two_textures_pipeline_layout,
                                       key.renderpass,
                                       0,
                                       VK_NULL_HANDLE,
                                       0}));
    return *blit_depth_stencil_pipelines.back();
}

VkPipeline BlitImageHelper::FindOrEmplaceClearColorPipeline(const BlitImagePipelineKey& key) {
    const auto it = std::ranges::find(clear_color_keys, key);
    if (it != clear_color_keys.end()) {
        return *clear_color_pipelines[std::distance(clear_color_keys.begin(), it)];
    }
    clear_color_keys.push_back(key);
    const std::array stages = MakeStages(*clear_color_vert, *clear_color_frag);
    const vk::PipelineColorBlendAttachmentState color_blend_attachment_state{
        VK_TRUE,
        vk::BlendFactor::eConstantColor,
        vk::BlendFactor::eOneMinusConstantColor,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eConstantAlpha,
        vk::BlendFactor::eOneMinusConstantAlpha,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    const vk::PipelineColorBlendStateCreateInfo color_blend_state_generic_create_info{
        {},
        VK_FALSE,
        vk::LogicOp::eClear,
        1,
        &color_blend_attachment_state,
        {0.0f, 0.0f, 0.0f, 0.0f},
    };
    clear_color_pipelines.push_back(device.logical().createPipeline(
        vk::GraphicsPipelineCreateInfo{{},
                                       stages,
                                       &PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                                       &PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                       nullptr,
                                       &PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                       &PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                       &PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                       &PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                       &color_blend_state_generic_create_info,
                                       &PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                       *clear_color_pipeline_layout,
                                       key.renderpass,
                                       0,
                                       VK_NULL_HANDLE,
                                       0}));
    return *clear_color_pipelines.back();
}

VkPipeline BlitImageHelper::FindOrEmplaceClearStencilPipeline(
    const BlitDepthStencilPipelineKey& key) {
    const auto it = std::ranges::find(clear_stencil_keys, key);
    if (it != clear_stencil_keys.end()) {
        return *clear_stencil_pipelines[std::distance(clear_stencil_keys.begin(), it)];
    }
    clear_stencil_keys.push_back(key);
    const std::array stages = MakeStages(*clear_color_vert, *clear_stencil_frag);
    const auto stencil = VkStencilOpState{
        .failOp = VK_STENCIL_OP_KEEP,
        .passOp = VK_STENCIL_OP_REPLACE,
        .depthFailOp = VK_STENCIL_OP_KEEP,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .compareMask = key.stencil_compare_mask,
        .writeMask = key.stencil_mask,
        .reference = key.stencil_ref,
    };
    const vk::PipelineDepthStencilStateCreateInfo depth_stencil_ci{
        {},
        key.depth_clear,
        key.depth_clear,
        vk::CompareOp::eAlways,
        VK_FALSE,
        VK_TRUE,
        stencil,
        stencil,
        0.0f,
        0.0f,
    };
    clear_stencil_pipelines.push_back(device.logical().createPipeline(
        vk::GraphicsPipelineCreateInfo{{},
                                       stages,
                                       &PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                                       &PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                       nullptr,
                                       &PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                       &PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                       &PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                       &depth_stencil_ci,
                                       &PIPELINE_COLOR_BLEND_STATE_GENERIC_CREATE_INFO,
                                       &PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                       *clear_color_pipeline_layout,
                                       key.renderpass,
                                       0,
                                       VK_NULL_HANDLE,
                                       0}));
    return *clear_stencil_pipelines.back();
}

void BlitImageHelper::ConvertPipeline(Pipeline& pipeline, VkRenderPass renderpass,
                                      bool is_target_depth) {
    if (pipeline) {
        return;
    }
    vk::ShaderModule frag_shader =
        is_target_depth ? *convert_float_to_depth_frag : *convert_depth_to_float_frag;
    const std::array stages = MakeStages(*full_screen_vert, frag_shader);
    pipeline = device.logical().createPipeline(vk::GraphicsPipelineCreateInfo{
        {},
        stages,
        &PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        &PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        nullptr,
        &PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        &PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        &PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        is_target_depth ? &PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO : nullptr,
        is_target_depth ? &PIPELINE_COLOR_BLEND_STATE_EMPTY_CREATE_INFO
                        : &PIPELINE_COLOR_BLEND_STATE_GENERIC_CREATE_INFO,
        &PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        *one_texture_pipeline_layout,
        renderpass,
        0,
        VK_NULL_HANDLE,
        0,
    });
}

void BlitImageHelper::ConvertDepthToColorPipeline(Pipeline& pipeline, VkRenderPass renderpass) {
    ConvertPipeline(pipeline, renderpass, false);
}

void BlitImageHelper::ConvertColorToDepthPipeline(Pipeline& pipeline, VkRenderPass renderpass) {
    ConvertPipeline(pipeline, renderpass, true);
}

void BlitImageHelper::ConvertPipelineEx(Pipeline& pipeline, VkRenderPass renderpass,
                                        ShaderModule& module, bool single_texture,
                                        bool is_target_depth) {
    if (pipeline) {
        return;
    }
    const std::array stages = MakeStages(*full_screen_vert, *module);
    pipeline = device.logical().createPipeline(vk::GraphicsPipelineCreateInfo{
        {},
        stages,
        &PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        &PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        nullptr,
        &PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        &PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        &PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        is_target_depth ? &PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO : nullptr,
        &PIPELINE_COLOR_BLEND_STATE_GENERIC_CREATE_INFO,
        &PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        single_texture ? *one_texture_pipeline_layout : *two_textures_pipeline_layout,
        renderpass,
        0,
        VK_NULL_HANDLE,
        0});
}

void BlitImageHelper::ConvertPipelineColorTargetEx(Pipeline& pipeline, VkRenderPass renderpass,
                                                   ShaderModule& module) {
    ConvertPipelineEx(pipeline, renderpass, module, false, false);
}

void BlitImageHelper::ConvertPipelineDepthTargetEx(Pipeline& pipeline, VkRenderPass renderpass,
                                                   ShaderModule& module) {
    ConvertPipelineEx(pipeline, renderpass, module, true, true);
}

}  // namespace render::vulkan
