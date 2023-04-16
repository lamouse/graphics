#ifndef G_RENDER_HPP
#define G_RENDER_HPP

#include "resource/shader_stage.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>
namespace g
{

    struct PipelineConfigInfo{
        ::vk::PipelineDepthStencilStateCreateInfo depthStencilStateInfo;
        ::vk::PipelineRasterizationStateCreateInfo rasterizationStateInfo;
        ::vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        ::vk::PipelineColorBlendAttachmentState colorBlendAttachsInfo;
        ::vk::PipelineMultisampleStateCreateInfo multisampleInfo;
        ::vk::PipelineViewportStateCreateInfo viewportStateInfo;
        ::vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo;
        ::vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
        ::std::vector<::vk::DynamicState> dynamicStateEnables;
        ::std::vector<::vk::VertexInputBindingDescription> bindingDescriptions{};
        ::std::vector<::vk::VertexInputAttributeDescription> attributeDescriptions{};
        ::std::vector<::vk::PipelineShaderStageCreateInfo> shaderStages;
        ::vk::RenderPass renderPass;
        ::vk::PipelineLayout layout;
    };
    class PipeLine
    {
    private:
        ::vk::Pipeline pipeline;
    public:
        PipeLine()=default;
        void initPipeline(::vk::Device& device, PipelineConfigInfo& configInfo);
        PipeLine(const PipeLine&) = delete;
        void bind(::vk::CommandBuffer& commandBuffer){commandBuffer.bindPipeline(::vk::PipelineBindPoint::eGraphics ,pipeline);};
        PipeLine operator=(const PipeLine&) = delete;
        PipeLine& operator=(PipeLine&&) = default;
        ::vk::Pipeline& operator()(){return pipeline;}
        void destroy(::vk::Device& device);
        static void enableAlphaBlending(PipelineConfigInfo& configInfo);
        static PipelineConfigInfo getDefaultConfig();

        ~PipeLine();
    };

}

#endif