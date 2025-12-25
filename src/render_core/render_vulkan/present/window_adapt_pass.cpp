#include "window_adapt_pass.hpp"
#include "render_core/render_vulkan/vk_graphic.hpp"
#include "present_push_constants.h"
#include "render_core/host_shaders/vulkan_present_vert_spv.h"
#include "render_vulkan/present_manager.hpp"
#include "render_core/render_vulkan/scheduler.hpp"
import render.vulkan.shader;
import render.vulkan.utils;
import render.vulkan.present.layer;
namespace render::vulkan::present {

namespace {}  // namespace

WindowAdaptPass::WindowAdaptPass(const Device& device_, vk::Format frame_format, Sampler&& sampler_,
                                 ShaderModule&& fragment_shader_)
    : device(device_), sampler(std::move(sampler_)), fragment_shader(std::move(fragment_shader_)) {
    CreateDescriptorSetLayout();
    CreatePipelineLayout();
    CreateVertexShader();
    CreatePipelines(frame_format);
}

WindowAdaptPass::~WindowAdaptPass() = default;

auto WindowAdaptPass::getDescriptorSetLayout() -> vk::DescriptorSetLayout {
    return *descriptor_set_layout;
}

void WindowAdaptPass::CreateVertexShader() {
    vertex_shader =
        ::render::vulkan::utils::buildShader(device.getLogical(), VULKAN_PRESENT_VERT_SPV);
}

void WindowAdaptPass::CreateDescriptorSetLayout() {
    descriptor_set_layout = utils::CreateWrappedDescriptorSetLayout(
        device, {vk::DescriptorType::eCombinedImageSampler});
}

void WindowAdaptPass::CreatePipelineLayout() {
    std::array ranges{vk::PushConstantRange{
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(PresentPushConstants),
    }};

    std::array layouts{*descriptor_set_layout};
    pipeline_layout =
        device.logical().createPipelineLayout(vk::PipelineLayoutCreateInfo{{}, layouts, ranges});
}

void WindowAdaptPass::CreatePipelines(vk::Format format) {
    opaque_pipeline = utils::CreateWrappedPipeline(device, format, pipeline_layout,
                                                   std::tie(vertex_shader, fragment_shader));
    premultiplied_pipeline = utils::CreateWrappedPremultipliedBlendingPipeline(
        device, format, pipeline_layout, std::tie(vertex_shader, fragment_shader));
    coverage_pipeline = utils::CreateWrappedCoverageBlendingPipeline(
        device, format, pipeline_layout, std::tie(vertex_shader, fragment_shader));
}

void WindowAdaptPass::Draw(VulkanGraphics& rasterizer, scheduler::Scheduler& scheduler,
                           size_t image_index, std::list<Layer>& layers,
                           std::span<const frame::FramebufferConfig> configs,
                           const layout::FrameBufferLayout& layout, Frame* dst) {
    const vk::PipelineLayout graphics_pipeline_layout{*pipeline_layout};
    const vk::ImageView image_view{*dst->image_view};
    const vk::Extent2D render_area{
        dst->width,
        dst->height,
    };

    const size_t layer_count = configs.size();
    std::vector<PresentPushConstants> push_constants(layer_count);
    std::vector<vk::DescriptorSet> descriptor_sets(layer_count);
    std::vector<vk::Pipeline> graphics_pipelines(layer_count);

    auto layer_it = layers.begin();
    for (size_t i = 0; i < layer_count; i++) {
        switch (configs[i].blending) {
            case render::frame::BlendMode::Opaque:
            default:
                graphics_pipelines[i] = *opaque_pipeline;
                break;
            case render::frame::BlendMode::Premultiplied:
                graphics_pipelines[i] = *premultiplied_pipeline;
                break;
            case render::frame::BlendMode::Coverage:
                graphics_pipelines[i] = *coverage_pipeline;
                break;
        }

        layer_it->ConfigureDraw(&push_constants[i], &descriptor_sets[i], rasterizer, *sampler,
                                image_index, configs[i], layout);
        layer_it++;
    }
    scheduler.record([=](vk::CommandBuffer cmdbuf) -> void {
        const f32 bg_red = 255.0F;
        const f32 bg_green = 255.0F;
        const f32 bg_blue = 255.0F;
        const vk::ClearAttachment clear_attachment =
            vk::ClearAttachment()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setColorAttachment(0)
                .setClearValue(vk::ClearValue().setColor(
                    vk::ClearColorValue().setFloat32({bg_red, bg_green, bg_blue, 1.0F})));

        const vk::ClearRect clear_rect =
            vk::ClearRect()
                .setRect(
                    vk::Rect2D().setOffset(vk::Offset2D().setX(0).setY(0)).setExtent(render_area))
                .setBaseArrayLayer(0)
                .setLayerCount(1);
        utils::BeginDynamicRendering(cmdbuf, image_view, render_area);
        cmdbuf.clearAttachments({clear_attachment}, {clear_rect});
        for (size_t i = 0; i < layer_count; i++) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipelines[i]);
            cmdbuf.pushConstants(graphics_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0,
                                 sizeof(push_constants[i]), &push_constants[i]);
            cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphics_pipeline_layout, 0,
                                      descriptor_sets[i], {});
            cmdbuf.draw(4, 1, 0, 0);
        }
        cmdbuf.endRendering();
    });
}

}  // namespace render::vulkan::present