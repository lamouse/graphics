#pragma once
#include "g_camera.hpp"
#include "g_pipeline.hpp"
#include "g_game_object.hpp"
#include "resource/image_texture.hpp"
#include "g_descriptor.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
namespace g
{
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
    class RenderSystem
    {
    private:
        ::vk::PipelineLayout pipelineLayout;
        
        PipeLine pipeline;
        Descriptor descriptor_;
        ::std::vector<::vk::Buffer> uniformBuffers;
        ::std::vector<::vk::DeviceMemory> uniformBuffersMemory;
        ::std::vector<void*> uniformBuffersMapped;
        ::std::vector<::vk::DescriptorSet> descriptorSets;
        void createPipelineLayout();
        void createPipeline(::vk::RenderPass renderPass);
        void createDescriptorSets(uint32_t count);
        void updateDescriptorSet(uint32_t currentImage, ::vk::ImageView imageView, ::vk::Sampler sampler);
    public:
        RenderSystem(::vk::RenderPass renderPass);
        void updateUniformBuffer(uint32_t currentImage, float extentAspectRation);
        void createUniformBuffers(uint32_t count);
        ::vk::DescriptorPool getDescriptorPool(){return descriptor_.getDescriptorPool(); }
        void renderGameObject(::std::vector<GameObject>& gameObjects, int currentFrame, ::vk::CommandBuffer commandBuffer, float extentAspectRation, 
                                resource::image::ImageTexture& imageTexture);
        ~RenderSystem();
};


}
