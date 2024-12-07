#pragma once
#include "g_frame.hpp"
#include "g_pipeline.hpp"
#include "core/device.hpp"

namespace g {
class RenderSystem {
    private:
        ::vk::PipelineLayout pipelineLayout;
        ::core::Device &device_;
        GraphicsPipeLine pipeline;
        void createPipelineLayout(::vk::DescriptorSetLayout descriptorSetLayout);
        void createPipeline(::vk::RenderPass renderPass);

    public:
        RenderSystem(const RenderSystem &) = delete;
        RenderSystem(RenderSystem &&) = delete;
        auto operator=(const RenderSystem &) -> RenderSystem & = delete;
        auto operator=(RenderSystem &&) -> RenderSystem & = delete;
        RenderSystem(::core::Device &device, ::vk::RenderPass renderPass,
                     ::vk::DescriptorSetLayout descriptorSetLayout);
        void render(FrameInfo &frameInfo) const;
        ~RenderSystem();
};

}  // namespace g
