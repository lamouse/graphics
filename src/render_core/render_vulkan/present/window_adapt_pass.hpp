#pragma once

#include <vulkan/vulkan.hpp>
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
namespace render::vulkan {
namespace present {
class WindowAdaptPass final {
    public:
        // explicit WindowAdaptPass(vk::Device device, VkFormat frame_format, vk::Sampler&& sampler,
        //                          vk::ShaderModule&& fragment_shader);
        // ~WindowAdaptPass();

        // void Draw(RasterizerVulkan& rasterizer, Scheduler& scheduler, size_t image_index,
        //           std::list<Layer>& layers, std::span<const Tegra::FramebufferConfig> configs,
        //           const Layout::FramebufferLayout& layout, Frame* dst);

        // auto GetDescriptorSetLayout() -> vk::DescriptorSetLayout;
        // auto getRenderPass() -> vk::RenderPass;
        auto create_vertex_shader(vk::Device device) -> ShaderModule;
        auto create_fragment_shader(vk::Device device) -> ShaderModule;

    private:
        // void CreateDescriptorSetLayout();
        // void CreatePipelineLayout();
        // void CreateVertexShader();
        // void CreateRenderPass(VkFormat frame_format);
        // void CreatePipelines();

    private:
        // const Device& device;
        vk::DescriptorSetLayout descriptor_set_layout;
        vk::PipelineLayout pipeline_layout;
        vk::Sampler sampler;
        vk::ShaderModule vertex_shader;
        vk::ShaderModule fragment_shader;
        vk::RenderPass render_pass;
        vk::Pipeline opaque_pipeline;
        vk::Pipeline premultiplied_pipeline;
        vk::Pipeline coverage_pipeline;
};
}  // namespace present

}  // namespace render::vulkan