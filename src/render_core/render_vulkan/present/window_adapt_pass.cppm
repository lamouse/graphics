module;

#include "render_core/framebuffer_config.hpp"
#include "core/frontend/framebuffer_layout.hpp"
#include <vulkan/vulkan.hpp>
#include <span>
#include <list>


export module render.vulkan:window_adapt_pass;

import :layer;
import render.vulkan.present.present_frame;
import render.vulkan.common;

export namespace render::vulkan {

namespace present {
class WindowAdaptPass final {
    public:
        explicit WindowAdaptPass(const Device& device, vk::Format frame_format, Sampler&& sampler,
                                 ShaderModule&& fragment_shader);
        ~WindowAdaptPass();

        auto getDescriptorSetLayout() -> vk::DescriptorSetLayout;
        void Draw(VulkanGraphics& rasterizer, scheduler::Scheduler& scheduler, size_t image_index,
                  std::list<Layer>& layers, std::span<const frame::FramebufferConfig> configs,
                  const layout::FrameBufferLayout& layout, Frame* dst);

    private:
        void CreateDescriptorSetLayout();
        void CreatePipelineLayout();
        void CreateVertexShader();
        void CreatePipelines(vk::Format format);

        const Device& device;
        DescriptorSetLayout descriptor_set_layout;
        PipelineLayout pipeline_layout;
        Sampler sampler;
        ShaderModule vertex_shader;
        ShaderModule fragment_shader;
        Pipeline opaque_pipeline;
        Pipeline premultiplied_pipeline;
        Pipeline coverage_pipeline;
};

}  // namespace present

}  // namespace render::vulkan