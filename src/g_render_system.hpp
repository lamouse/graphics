#pragma once
#include "g_camera.hpp"
#include "g_pipeline.hpp"
#include <memory>
#include <vulkan/vulkan_handles.hpp>
namespace g
{
    class RenderSystem
    {
    private:
        ::vk::PipelineLayout pipelineLayout;
        ::std::unique_ptr<PipeLine> pipeline;
        void createPipelineLayout();
        void createPipeline(::vk::RenderPass renderPass);
    public:
        RenderSystem(::vk::RenderPass renderPass);
        void renderGameObject(::std::vector<GameObject>& gameObjects, const Camera& camera, ::vk::CommandBuffer commandBuffer);
        ~RenderSystem();
};


}
