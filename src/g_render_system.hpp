#pragma once
#include "g_camera.hpp"
#include "g_pipeline.hpp"
#include <memory>
#include <vulkan/vulkan_handles.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
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
        ::vk::DescriptorSetLayout setLayout;
        ::std::unique_ptr<PipeLine> pipeline;
        ::std::vector<::vk::Buffer> uniformBuffers;
        ::std::vector<::vk::DeviceMemory> uniformBuffersMemory;
        ::std::vector<void*> uniformBuffersMapped;
        ::std::vector<::vk::DescriptorSet> descriptorSets;
        ::vk::DescriptorPool descriptorPool;
        void createPipelineLayout();
        void createPipeline(::vk::RenderPass renderPass);
        void createDescriptorPool(uint32_t count);
        void createDescriptorSets(uint32_t count);
    public:
        RenderSystem(::vk::RenderPass renderPass);
        void updateUniformBuffer(uint32_t currentImage, float extentAspectRation);
        void createUniformBuffers(uint32_t count);
        void renderGameObject(::std::vector<GameObject>& gameObjects, int currentFrame, ::vk::CommandBuffer commandBuffer, float extentAspectRation);
        ~RenderSystem();
};


}
