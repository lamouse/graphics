#include "window_adapt_pass.hpp"
#include "present_push_constants.h"
#include "render_core/host_shaders/vulkan_present_vert_spv.h"
#include "render_vulkan/vk_shader_util.hpp"
#include "render_vulkan/present_manager.hpp"
#include "vulkan_utils.hpp"
#include "layer.hpp"
#include "render_core/render_vulkan/scheduler.hpp"
#include "shader_tools/shader_compile.hpp"
#include "render_core/uniforms.hpp"
namespace render::vulkan::present {

namespace {

auto getBindingDescription() -> ::std::vector<::vk::VertexInputBindingDescription> {
    ::std::vector<::vk::VertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].setBinding(0);
    bindingDescriptions[0].setStride(sizeof(Vertex));

    return bindingDescriptions;
}

auto getAttributeDescription() -> ::std::vector<::vk::VertexInputAttributeDescription> {
    ::std::vector<::vk::VertexInputAttributeDescription> attributeDescriptions(3);
    attributeDescriptions[0].setBinding(0);
    attributeDescriptions[0].setLocation(0);
    attributeDescriptions[0].setFormat(::vk::Format::eR32G32B32Sfloat);
    attributeDescriptions[0].setOffset(offsetof(Vertex, position));

    attributeDescriptions[1].setBinding(0);
    attributeDescriptions[1].setLocation(1);
    attributeDescriptions[1].setFormat(::vk::Format::eR32G32B32Sfloat);
    attributeDescriptions[1].setOffset(offsetof(Vertex, color));

    attributeDescriptions[2].setBinding(0);
    attributeDescriptions[2].setLocation(2);
    attributeDescriptions[2].setFormat(::vk::Format::eR32G32Sfloat);
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

}  // namespace

WindowAdaptPass::WindowAdaptPass(const Device& device_, vk::Format frame_format, Sampler&& sampler_,
                                 ShaderModule&& fragment_shader_)
    : device(device_), sampler(std::move(sampler_)), fragment_shader(std::move(fragment_shader_)) {
    spdlog::debug("初始化window adapt pass");
    CreateDescriptorSetLayout();
    CreatePipelineLayout();
    CreateVertexShader();
    CreateRenderPass(frame_format);
    CreatePipelines();
}

WindowAdaptPass::~WindowAdaptPass() = default;

auto WindowAdaptPass::getDescriptorSetLayout() -> vk::DescriptorSetLayout {
    return *descriptor_set_layout;
}

auto WindowAdaptPass::getRenderPass() -> vk::RenderPass { return *render_pass; }
void WindowAdaptPass::CreateVertexShader() {
    shader::compile::printShaderAttributes(VULKAN_PRESENT_VERT_SPV);
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

void WindowAdaptPass::CreateRenderPass(vk::Format frame_format) {
    render_pass = utils::CreateWrappedRenderPass(device, frame_format, vk::ImageLayout::eUndefined);
}

void WindowAdaptPass::CreatePipelines() {
    spdlog::debug("WindowAdaptPass 创建pipeline");
    opaque_pipeline = utils::CreateWrappedPipeline(device, render_pass, pipeline_layout,
                                                   std::tie(vertex_shader, fragment_shader));
    premultiplied_pipeline = utils::CreateWrappedPremultipliedBlendingPipeline(
        device, render_pass, pipeline_layout, std::tie(vertex_shader, fragment_shader));
    coverage_pipeline = utils::CreateWrappedCoverageBlendingPipeline(
        device, render_pass, pipeline_layout, std::tie(vertex_shader, fragment_shader));
}

void WindowAdaptPass::Draw(scheduler::Scheduler& scheduler, size_t image_index,
                           std::list<Layer>& layers,
                           std::span<const frame::FramebufferConfig> configs,
                           const layout::FrameBufferLayout& layout, Frame* dst) {
    spdlog::debug("WindowAdaptPass 执行Draw image index: {}", image_index);
    const vk::Framebuffer host_framebuffer{*dst->framebuffer};
    const vk::RenderPass renderpass{*render_pass};
    const vk::PipelineLayout graphics_pipeline_layout{*pipeline_layout};
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
        }  //(&push_constants[i], &descriptor_sets[i], rasterizer, *sampler,  image_index,
           // configs[i], layout)

        layer_it->ConfigureDraw(&push_constants[i], &descriptor_sets[i], *sampler, image_index,
                                configs[i], layout);
        layer_it++;
    }

    scheduler.record([=](vk::CommandBuffer cmdbuf) {
        const f32 bg_red = 255.0f;  // TODO 这里有设置作用未知
        const f32 bg_green = 255.0f;
        const f32 bg_blue = 255.0f;
        const VkClearAttachment clear_attachment{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .colorAttachment = 0,
            .clearValue =
                {
                    .color = {.float32 = {bg_red, bg_green, bg_blue, 1.0f}},
                },
        };
        const VkClearRect clear_rect{
            .rect =
                {
                    .offset = {0, 0},
                    .extent = render_area,
                },
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        utils::BeginRenderPass(cmdbuf, renderpass, host_framebuffer, render_area);
        cmdbuf.clearAttachments({clear_attachment}, {clear_rect});

        for (size_t i = 0; i < layer_count; i++) {
            cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipelines[i]);
            cmdbuf.pushConstants(graphics_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0,
                                 sizeof(push_constants[i]), &push_constants[i]);
            cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphics_pipeline_layout, 0,
                                      descriptor_sets[i], {});
            cmdbuf.draw(4, 1, 0, 0);
        }

        cmdbuf.endRenderPass();
    });
}

}  // namespace render::vulkan::present