#pragma once

#include <vulkan/vulkan.hpp>
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
namespace render::vulkan {
class Device;
struct Frame;

namespace scheduler {
class Scheduler;
}
namespace present {
class WindowAdaptPass final {
    public:
        explicit WindowAdaptPass(const Device& device, vk::Format frame_format, Sampler&& sampler,
                                 ShaderModule&& fragment_shader);
        ~WindowAdaptPass();

        // void Draw(RasterizerVulkan& rasterizer, scheduler::Scheduler& scheduler, size_t
        // image_index,
        //           std::list<Layer>& layers,
        //           const Layout::FramebufferLayout& layout, Frame* dst);

        auto getDescriptorSetLayout() -> vk::DescriptorSetLayout;
        auto getRenderPass() -> vk::RenderPass;

    private:
        void CreateDescriptorSetLayout();
        void CreatePipelineLayout();
        void CreateVertexShader();
        void CreateRenderPass(vk::Format frame_format);
        void CreatePipelines();

        const Device& device;
        DescriptorSetLayout descriptor_set_layout;
        PipelineLayout pipeline_layout;
        Sampler sampler;
        ShaderModule vertex_shader;
        ShaderModule fragment_shader;
        RenderPass render_pass;
        Pipeline opaque_pipeline;
        Pipeline premultiplied_pipeline;
        Pipeline coverage_pipeline;
};

}  // namespace present

}  // namespace render::vulkan