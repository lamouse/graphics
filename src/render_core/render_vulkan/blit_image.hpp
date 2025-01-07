// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "descriptor_pool.hpp"
#include "texture/types.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"

namespace render::vulkan {

enum class Operation : u32 {
    SrcCopyAnd = 0,
    ROPAnd = 1,
    Blend = 2,
    SrcCopy = 3,
    ROP = 4,
    SrcCopyPremult = 5,
    BlendPremult = 6,
};
using texture::Extent3D;
using texture::Offset2D;
using texture::Region2D;

class Device;
class TextureFramebuffer;
class TextureImageView;
namespace scheduler {
class Scheduler;
}

struct BlitImagePipelineKey {
        constexpr auto operator<=>(const BlitImagePipelineKey&) const noexcept = default;
        Operation operation;
        vk::RenderPass renderpass;
};

struct BlitDepthStencilPipelineKey {
        constexpr auto operator<=>(const BlitDepthStencilPipelineKey&) const noexcept = default;

        vk::RenderPass renderpass;
        bool depth_clear;
        u8 stencil_mask;
        u32 stencil_compare_mask;
        u32 stencil_ref;
};

class BlitImageHelper {
    public:
        explicit BlitImageHelper(const Device& device, scheduler::Scheduler& scheduler,
                                 resource::DescriptorPool& descriptor_pool);
        ~BlitImageHelper();

        void BlitColor(const TextureFramebuffer* dst_framebuffer, VkImageView src_image_view,
                       const Region2D& dst_region, const Region2D& src_region, Operation operation);

        void BlitColor(const TextureFramebuffer* dst_framebuffer, VkImageView src_image_view,
                       VkImage src_image, VkSampler src_sampler, const Region2D& dst_region,
                       const Region2D& src_region, const Extent3D& src_size);

        void BlitDepthStencil(const TextureFramebuffer* dst_framebuffer, VkImageView src_depth_view,
                              VkImageView src_stencil_view, const Region2D& dst_region,
                              const Region2D& src_region);

        void ConvertD32ToR32(const TextureFramebuffer* dst_framebuffer,
                             const TextureImageView& src_image_view);

        void ConvertR32ToD32(const TextureFramebuffer* dst_framebuffer,
                             const TextureImageView& src_image_view);

        void ConvertD16ToR16(const TextureFramebuffer* dst_framebuffer,
                             const TextureImageView& src_image_view);

        void ConvertR16ToD16(const TextureFramebuffer* dst_framebuffer,
                             const TextureImageView& src_image_view);

        void ConvertABGR8ToD24S8(const TextureFramebuffer* dst_framebuffer,
                                 const TextureImageView& src_image_view);

        void ConvertABGR8ToD32F(const TextureFramebuffer* dst_framebuffer,
                                const TextureImageView& src_image_view);

        void ConvertD32FToABGR8(const TextureFramebuffer* dst_framebuffer,
                                TextureImageView& src_image_view);

        void ConvertD24S8ToABGR8(const TextureFramebuffer* dst_framebuffer,
                                 TextureImageView& src_image_view);

        void ConvertS8D24ToABGR8(const TextureFramebuffer* dst_framebuffer,
                                 TextureImageView& src_image_view);

        void ClearColor(const TextureFramebuffer* dst_framebuffer, u8 color_mask,
                        const std::array<f32, 4>& clear_color, const Region2D& dst_region);

        void ClearDepthStencil(const TextureFramebuffer* dst_framebuffer, bool depth_clear,
                               f32 clear_depth, u8 stencil_mask, u32 stencil_ref,
                               u32 stencil_compare_mask, const Region2D& dst_region);

    private:
        void Convert(VkPipeline pipeline, const TextureFramebuffer* dst_framebuffer,
                     const TextureImageView& src_image_view);

        void ConvertDepthStencil(VkPipeline pipeline, const TextureFramebuffer* dst_framebuffer,
                                 TextureImageView& src_image_view);

        [[nodiscard]] VkPipeline FindOrEmplaceColorPipeline(const BlitImagePipelineKey& key);

        [[nodiscard]] VkPipeline FindOrEmplaceDepthStencilPipeline(const BlitImagePipelineKey& key);

        [[nodiscard]] VkPipeline FindOrEmplaceClearColorPipeline(const BlitImagePipelineKey& key);
        [[nodiscard]] VkPipeline FindOrEmplaceClearStencilPipeline(
            const BlitDepthStencilPipelineKey& key);

        void ConvertPipeline(Pipeline& pipeline, VkRenderPass renderpass, bool is_target_depth);

        void ConvertDepthToColorPipeline(Pipeline& pipeline, VkRenderPass renderpass);

        void ConvertColorToDepthPipeline(Pipeline& pipeline, VkRenderPass renderpass);

        void ConvertPipelineEx(Pipeline& pipeline, VkRenderPass renderpass, ShaderModule& module,
                               bool single_texture, bool is_target_depth);

        void ConvertPipelineColorTargetEx(Pipeline& pipeline, VkRenderPass renderpass,
                                          ShaderModule& module);

        void ConvertPipelineDepthTargetEx(Pipeline& pipeline, VkRenderPass renderpass,
                                          ShaderModule& module);

        const Device& device;
        scheduler::Scheduler& scheduler;

        DescriptorSetLayout one_texture_set_layout;
        DescriptorSetLayout two_textures_set_layout;
        resource::DescriptorAllocator one_texture_descriptor_allocator;
        resource::DescriptorAllocator two_textures_descriptor_allocator;
        PipelineLayout one_texture_pipeline_layout;
        PipelineLayout two_textures_pipeline_layout;
        PipelineLayout clear_color_pipeline_layout;
        ShaderModule full_screen_vert;
        ShaderModule blit_color_to_color_frag;
        ShaderModule blit_depth_stencil_frag;
        ShaderModule clear_color_vert;
        ShaderModule clear_color_frag;
        ShaderModule clear_stencil_frag;
        ShaderModule convert_depth_to_float_frag;
        ShaderModule convert_float_to_depth_frag;
        ShaderModule convert_abgr8_to_d24s8_frag;
        ShaderModule convert_abgr8_to_d32f_frag;
        ShaderModule convert_d32f_to_abgr8_frag;
        ShaderModule convert_d24s8_to_abgr8_frag;
        ShaderModule convert_s8d24_to_abgr8_frag;
        Sampler linear_sampler;
        Sampler nearest_sampler;

        std::vector<BlitImagePipelineKey> blit_color_keys;
        std::vector<Pipeline> blit_color_pipelines;
        std::vector<BlitImagePipelineKey> blit_depth_stencil_keys;
        std::vector<Pipeline> blit_depth_stencil_pipelines;
        std::vector<BlitImagePipelineKey> clear_color_keys;
        std::vector<Pipeline> clear_color_pipelines;
        std::vector<BlitDepthStencilPipelineKey> clear_stencil_keys;
        std::vector<Pipeline> clear_stencil_pipelines;
        Pipeline convert_d32_to_r32_pipeline;
        Pipeline convert_r32_to_d32_pipeline;
        Pipeline convert_d16_to_r16_pipeline;
        Pipeline convert_r16_to_d16_pipeline;
        Pipeline convert_abgr8_to_d24s8_pipeline;
        Pipeline convert_abgr8_to_d32f_pipeline;
        Pipeline convert_d32f_to_abgr8_pipeline;
        Pipeline convert_d24s8_to_abgr8_pipeline;
        Pipeline convert_s8d24_to_abgr8_pipeline;
};

}  // namespace render::vulkan
