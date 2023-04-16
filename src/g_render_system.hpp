#pragma once
#include "g_pipeline.hpp"
#include "resource/image_texture.hpp"
#include "g_descriptor.hpp"
#include "g_frame.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
namespace g
{
    class RenderSystem
    {
    private:
        ::vk::PipelineLayout pipelineLayout;
        
        PipeLine pipeline;
        Descriptor descriptor_;
        void createPipelineLayout(::vk::DescriptorSetLayout descriptorSetLayout);
        void createPipeline(::vk::RenderPass renderPass);
        void createDescriptorSets(uint32_t count);
        void updateDescriptorSet(uint32_t currentImage, ::vk::ImageView imageView, ::vk::Sampler sampler);
    public:
        RenderSystem(::vk::RenderPass renderPass, ::vk::DescriptorSetLayout descriptorSetLayout);
        ::vk::DescriptorPool getDescriptorPool(){return descriptor_.getDescriptorPool(); }
        void render(FrameInfo& frameInfo);
        ~RenderSystem();
};


}
