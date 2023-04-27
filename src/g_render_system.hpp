#pragma once
#include "g_pipeline.hpp"
#include "g_frame.hpp"
namespace g
{
    class RenderSystem
    {
    private:
        ::vk::PipelineLayout pipelineLayout;
        
        PipeLine pipeline;
        void createPipelineLayout(::vk::DescriptorSetLayout descriptorSetLayout);
        void createPipeline(::vk::RenderPass renderPass);
        void createDescriptorSets(uint32_t count);
        void updateDescriptorSet(uint32_t currentImage, ::vk::ImageView imageView, ::vk::Sampler sampler);
    public:
      RenderSystem(const RenderSystem &) = delete;
      RenderSystem(RenderSystem &&) = delete;
      auto operator=(const RenderSystem &) -> RenderSystem & = delete;
      auto operator=(RenderSystem &&) -> RenderSystem & = delete;
      RenderSystem(::vk::RenderPass renderPass,
                   ::vk::DescriptorSetLayout descriptorSetLayout);
      void render(FrameInfo &frameInfo);
      ~RenderSystem();
};


} // namespace g 
