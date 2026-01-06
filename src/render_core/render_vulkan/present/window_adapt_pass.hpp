#pragma once

#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include "render_core/framebuffer_config.hpp"
#include <list>
#include "core/frontend/framebuffer_layout.hpp"
#include "render_core/render_vulkan/present/present_frame.hpp"
#include <functional>
namespace render::vulkan {
class Device;
struct Frame;
class Layer;
namespace scheduler {
class Scheduler;
}
namespace present {
class WindowAdaptPass final {
    public:
        explicit WindowAdaptPass(const Device& device, vk::Format frame_format, Sampler&& sampler,
                                 ShaderModule&& fragment_shader);
        ~WindowAdaptPass();

        auto getDescriptorSetLayout() -> vk::DescriptorSetLayout;
        void Draw(
            const std::function<std::optional<present::FramebufferTextureInfo>(
                const frame::FramebufferConfig& framebuffer, uint32_t stride)>& accelerateDisplay,
            scheduler::Scheduler& scheduler, size_t image_index, std::list<Layer>& layers,
            std::span<const frame::FramebufferConfig> configs,
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