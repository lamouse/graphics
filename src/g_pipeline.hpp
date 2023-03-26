#ifndef G_RENDER_HPP
#define G_RENDER_HPP

#include "g_command.hpp"
#include "g_shader.hpp"
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
    };
    class PipeLine
    {
    private:
        ::vk::Pipeline pipeline;
        void initPipeline(::vk::RenderPass& renderPass, ::vk::PipelineLayout layout);
        PipelineConfigInfo configInfo;
        int currentFrame;
        ::std::unique_ptr<Shader> shader;

        void defaultPiplineConfig();
        void setDepthStencilStateConfig();
        void setRasterizationStateInfo();
        void setColorBlendInfo();
        void setMultisampleInfo();
        void setViewportStateInfo();
        void setInputAssemblyStateInfo();
        void setDynamicStateEnables();
    public:
        PipeLine(const ::std::string& vertFilePath, const ::std::string& fragFilePath, ::vk::RenderPass& renderPass, ::vk::PipelineLayout layout);
        PipeLine(const PipeLine&) = delete;
        void bind(::vk::CommandBuffer& commandBuffer){commandBuffer.bindPipeline(::vk::PipelineBindPoint::eGraphics ,pipeline);};
        PipeLine& operator=(const PipeLine&) = delete;
        void enableAlphaBlending(PipelineConfigInfo& configInfo);
        ~PipeLine();
    };

}

#endif