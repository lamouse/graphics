#pragma once
#include <vulkan/vulkan.hpp>
#include "common/common_funcs.hpp"
namespace render::vulkan {
class Device;
namespace scheduler {
class Scheduler;
}
namespace pipeline {
class GraphicsPipeline {
        static constexpr size_t NUM_STAGES = 5;

    public:
        // explicit GraphicsPipeline(
        //     scheduler::Scheduler& scheduler, BufferCache& buffer_cache, TextureCache& texture_cache,
        //     vk::PipelineCache& pipeline_cache, VideoCore::ShaderNotify* shader_notify,
        //     const Device& device, DescriptorPool& descriptor_pool,
        //     GuestDescriptorQueue& guest_descriptor_queue, Common::ThreadWorker* worker_thread,
        //     PipelineStatistics* pipeline_statistics, RenderPassCache& render_pass_cache,
        //     const GraphicsPipelineCacheKey& key, std::array<vk::ShaderModule, NUM_STAGES> stages,
        //     const std::array<const Shader::Info*, NUM_STAGES>& infos);

        CLASS_NON_COPYABLE(GraphicsPipeline);
        CLASS_NON_MOVEABLE(GraphicsPipeline);

    private:
        template <typename Spec>
        void ConfigureImpl(bool is_indexed);

        // void ConfigureDraw(const RescalingPushConstant& rescaling,
        //                 const RenderAreaPushConstant& render_are);

        void MakePipeline(VkRenderPass render_pass);

        void Validate();
};
}  // namespace pipeline
}  // namespace render::vulkan