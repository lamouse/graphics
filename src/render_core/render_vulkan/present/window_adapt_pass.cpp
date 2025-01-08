#include "window_adapt_pass.hpp"
#include "present_push_constants.h"
#include "render_core/host_shaders/vulkan_present_vert_spv.h"
#include "render_vulkan/vk_shader_util.hpp"
#include "render_vulkan/present_manager.hpp"
#include "vulkan_utils.hpp"
namespace render::vulkan::present {

WindowAdaptPass::WindowAdaptPass(const Device& device_, vk::Format frame_format, Sampler&& sampler_,
                                 ShaderModule&& fragment_shader_)
    : device(device_), sampler(std::move(sampler_)), fragment_shader(std::move(fragment_shader_)) {
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
    opaque_pipeline = utils::CreateWrappedPipeline(device, render_pass, pipeline_layout,
                                                   std::tie(vertex_shader, fragment_shader));
    premultiplied_pipeline = utils::CreateWrappedPremultipliedBlendingPipeline(
        device, render_pass, pipeline_layout, std::tie(vertex_shader, fragment_shader));
    coverage_pipeline = utils::CreateWrappedCoverageBlendingPipeline(
        device, render_pass, pipeline_layout, std::tie(vertex_shader, fragment_shader));
}

}  // namespace render::vulkan::present