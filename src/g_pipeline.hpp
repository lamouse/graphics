#pragma once

#include <vulkan/vulkan.hpp>

namespace g {

struct PipelineConfigInfo {
        ::vk::PipelineDepthStencilStateCreateInfo depthStencilStateInfo;
        ::vk::PipelineRasterizationStateCreateInfo rasterizationStateInfo;
        ::vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        ::vk::PipelineColorBlendAttachmentState colorBlendAttachmentInfo;
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
class GraphicsPipeLine {
    private:
        ::vk::Pipeline pipeline;
    public:
        explicit GraphicsPipeLine(PipelineConfigInfo& configInfo);
        GraphicsPipeLine();
        GraphicsPipeLine(const GraphicsPipeLine&) = delete;
        void bind(const ::vk::CommandBuffer& commandBuffer)const {
            commandBuffer.bindPipeline(::vk::PipelineBindPoint::eGraphics, pipeline);
        }
        auto operator=(const GraphicsPipeLine&) -> GraphicsPipeLine = delete;
        auto operator=(GraphicsPipeLine&&)  noexcept -> GraphicsPipeLine&;
        GraphicsPipeLine(GraphicsPipeLine&&) noexcept ;
        auto operator()() -> ::vk::Pipeline& { return pipeline; }
        static void enableAlphaBlending(PipelineConfigInfo& configInfo);
        static auto getDefaultConfig() -> PipelineConfigInfo;

        ~GraphicsPipeLine();
};

class ComputePipeline {
    private:
        ::vk::Pipeline pipeline_;
        ::vk::PipelineLayout pipelineLayout_;
        void createPipelineLayout(const ::vk::DescriptorSetLayout& descriptorSetLayout);
        void createPipeline(const ::vk::PipelineShaderStageCreateInfo& shaderStageCreateInfo);

    public:
        void init(const ::vk::PipelineShaderStageCreateInfo& shaderStageCreateInfo,
                  const ::vk::DescriptorSetLayout& descriptorSetLayout);
        void destroy() const;
};

}  // namespace g
