module;
#include "render_vulkan/scheduler.hpp"
#include "common/settings.hpp"
#include "common/math_util.h"
#include "render_core/fsr.h"
#include "render_core/host_shaders/vulkan_fidelityfx_fsr_easu_fp16_frag_spv.h"
#include "render_core/host_shaders/vulkan_fidelityfx_fsr_easu_fp32_frag_spv.h"
#include "render_core/host_shaders/vulkan_fidelityfx_fsr_rcas_fp16_frag_spv.h"
#include "render_core/host_shaders/vulkan_fidelityfx_fsr_rcas_fp32_frag_spv.h"
#include "render_core/host_shaders/vulkan_fidelityfx_fsr_vert_spv.h"
module render.vulkan.FSR;
import render.vulkan.common;
import render.vulkan.shader;
import render.vulkan.utils;

namespace render::vulkan {
using PushConstants = std::array<u32, 4 * 4>;  // NOLINT

FSR::FSR(const Device& device, MemoryAllocator& memory_allocator, size_t image_count,
         vk::Extent2D extent)
    : m_device{device},
      m_memory_allocator{memory_allocator},
      m_image_count{image_count},
      m_extent{extent} {
    CreateImages();
    CreateSampler();
    CreateShaders();
    CreateDescriptorPool();
    CreateDescriptorSetLayout();
    CreateDescriptorSets();
    CreatePipelineLayouts();
    CreatePipelines();
}

void FSR::CreateImages() {
    m_dynamic_images.resize(m_image_count);
    for (auto& images : m_dynamic_images) {
        images.images[Easu] =
            present::utils::CreateWrappedImage(m_memory_allocator, m_extent, format_);
        images.images[Rcas] =
            present::utils::CreateWrappedImage(m_memory_allocator, m_extent, format_);
        images.image_views[Easu] =
            present::utils::CreateWrappedImageView(m_device, images.images[Easu], format_);
        images.image_views[Rcas] =
            present::utils::CreateWrappedImageView(m_device, images.images[Rcas], format_);
    }
}

void FSR::CreateSampler() { m_sampler = present::utils::CreateBilinearSampler(m_device); }

void FSR::CreateShaders() {
    m_vert_shader = utils::buildShader(m_device.getLogical(), VULKAN_FIDELITYFX_FSR_VERT_SPV);

    if (m_device.isFloat16Supported()) {
        m_easu_shader =
            utils::buildShader(m_device.getLogical(), VULKAN_FIDELITYFX_FSR_EASU_FP16_FRAG_SPV);
        m_rcas_shader =
            utils::buildShader(m_device.getLogical(), VULKAN_FIDELITYFX_FSR_RCAS_FP16_FRAG_SPV);
    } else {
        m_easu_shader =
            utils::buildShader(m_device.getLogical(), VULKAN_FIDELITYFX_FSR_EASU_FP32_FRAG_SPV);
        m_rcas_shader =
            utils::buildShader(m_device.getLogical(), VULKAN_FIDELITYFX_FSR_RCAS_FP32_FRAG_SPV);
    }
}

void FSR::CreateDescriptorPool() {
    // EASU: 1 descriptor
    // RCAS: 1 descriptor
    // 2 descriptors, 2 descriptor sets per invocation
    m_descriptor_pool =
        present::utils::CreateWrappedDescriptorPool(m_device, 2 * m_image_count, 2 * m_image_count);
}

void FSR::CreateDescriptorSetLayout() {
    m_descriptor_set_layout = present::utils::CreateWrappedDescriptorSetLayout(
        m_device, {vk::DescriptorType::eCombinedImageSampler});
}

void FSR::CreateDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(MaxFsrStage, *m_descriptor_set_layout);

    for (auto& images : m_dynamic_images) {
        images.descriptor_sets =
            present::utils::CreateWrappedDescriptorSets(m_descriptor_pool, layouts);
    }
}

void FSR::CreatePipelineLayouts() {
    const vk::PushConstantRange range{
        vk::ShaderStageFlagBits::eFragment,
        0,
        sizeof(PushConstants),
    };
    vk::PipelineLayoutCreateInfo ci{
        {}, 1, m_descriptor_set_layout.address(), 1, &range,
    };

    m_pipeline_layout = m_device.logical().createPipelineLayout(ci);
}

void FSR::CreatePipelines() {
    m_easu_pipeline = present::utils::CreateWrappedPipeline(m_device, format_, m_pipeline_layout,
                                                            std::tie(m_vert_shader, m_easu_shader));
    m_rcas_pipeline = present::utils::CreateWrappedPipeline(m_device, format_, m_pipeline_layout,
                                                            std::tie(m_vert_shader, m_rcas_shader));
}

void FSR::UpdateDescriptorSets(vk::ImageView image_view, size_t image_index) {
    Images& images = m_dynamic_images[image_index];
    std::vector<vk::DescriptorImageInfo> image_infos;
    std::vector<vk::WriteDescriptorSet> updates;
    image_infos.reserve(2);

    updates.push_back(present::utils::CreateWriteDescriptorSet(image_infos, *m_sampler, image_view,
                                                               images.descriptor_sets[Easu], 0));
    updates.push_back(present::utils::CreateWriteDescriptorSet(
        image_infos, *m_sampler, *images.image_views[Easu], images.descriptor_sets[Rcas], 0));

    m_device.getLogical().updateDescriptorSets(updates, {});
}

void FSR::UploadImages(scheduler::Scheduler& scheduler) {
    if (m_images_ready) {
        return;
    }

    scheduler.record([&](vk::CommandBuffer cmdbuf) {
        for (auto& image : m_dynamic_images) {
            present::utils::ClearColorImage(cmdbuf, *image.images[Easu]);
            present::utils::ClearColorImage(cmdbuf, *image.images[Rcas]);
        }
    });
    scheduler.finish();

    m_images_ready = true;
}

auto FSR::Draw(scheduler::Scheduler& scheduler, size_t image_index, vk::Image source_image,
               vk::ImageView source_image_view, vk::Extent2D input_image_extent,
               const common::Rectangle<f32>& crop_rect) -> vk::ImageView {
    Images& images = m_dynamic_images[image_index];

    vk::Image easu_image = *images.images[Easu];
    vk::Image rcas_image = *images.images[Rcas];
    vk::DescriptorSet easu_descriptor_set = images.descriptor_sets[Easu];
    vk::DescriptorSet rcas_descriptor_set = images.descriptor_sets[Rcas];

    vk::ImageView easu_image_view = *images.image_views[Easu];
    vk::ImageView rcas_image_view = *images.image_views[Rcas];

    vk::Pipeline easu_pipeline = *m_easu_pipeline;
    vk::Pipeline rcas_pipeline = *m_rcas_pipeline;
    vk::PipelineLayout pipeline_layout = *m_pipeline_layout;
    vk::Extent2D extent = m_extent;

    const f32 input_image_width = static_cast<f32>(input_image_extent.width);
    const f32 input_image_height = static_cast<f32>(input_image_extent.height);
    const f32 output_image_width = static_cast<f32>(extent.width);
    const f32 output_image_height = static_cast<f32>(extent.height);
    const f32 viewport_width = (crop_rect.right - crop_rect.left) * input_image_width;
    const f32 viewport_x = crop_rect.left * input_image_width;
    const f32 viewport_height = (crop_rect.bottom - crop_rect.top) * input_image_height;
    const f32 viewport_y = crop_rect.top * input_image_height;

    PushConstants easu_con{};
    PushConstants rcas_con{};
    ::FSR::FsrEasuConOffset(easu_con.data() + 0, easu_con.data() + 4, easu_con.data() + 8,
                            easu_con.data() + 12, viewport_width, viewport_height,
                            input_image_width, input_image_height, output_image_width,
                            output_image_height, viewport_x, viewport_y);

    const float sharpening =
        static_cast<float>(settings::values.fsr_sharpening_slider.GetValue()) / 100.0F;
    ::FSR::FsrRcasCon(rcas_con.data(), sharpening);

    UploadImages(scheduler);
    UpdateDescriptorSets(source_image_view, image_index);

    scheduler.requestOutsideRenderOperationContext();
    scheduler.record([=](vk::CommandBuffer cmdbuf) {
        present::utils::TransitionImageLayout(cmdbuf, source_image, vk::ImageLayout::eGeneral);
        present::utils::TransitionImageLayout(cmdbuf, easu_image, vk::ImageLayout::eGeneral);

        present::utils::BeginDynamicRendering(cmdbuf, easu_image_view, extent);
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, easu_pipeline);
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0,
                                  easu_descriptor_set, {});
        cmdbuf.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0,
                             sizeof(easu_con), &easu_con);
        cmdbuf.draw(3, 1, 0, 0);
        cmdbuf.endRendering();

        present::utils::TransitionImageLayout(cmdbuf, easu_image, vk::ImageLayout::eGeneral);
        present::utils::TransitionImageLayout(cmdbuf, rcas_image, vk::ImageLayout::eGeneral);
        present::utils::BeginDynamicRendering(cmdbuf, rcas_image_view, extent);
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, rcas_pipeline);
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0,
                                  rcas_descriptor_set, {});
        cmdbuf.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0,
                             sizeof(rcas_con), &rcas_con);
        cmdbuf.draw(3, 1, 0, 0);
        cmdbuf.endRendering();

        present::utils::TransitionImageLayout(cmdbuf, rcas_image, vk::ImageLayout::eGeneral);
    });

    return *images.image_views[Rcas];
}

}  // namespace render::vulkan