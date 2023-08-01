#pragma once

#include <vulkan/vulkan.hpp>

namespace g {

struct PipelineConfigInfo {
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
class GraphicsPipeLine {
    private:
        ::vk::Pipeline pipeline;

    public:
        GraphicsPipeLine() = default;
        void initPipeline(::vk::Device& device, PipelineConfigInfo& configInfo);
        GraphicsPipeLine(const GraphicsPipeLine&) = delete;
        void bind(::vk::CommandBuffer& commandBuffer) {
            commandBuffer.bindPipeline(::vk::PipelineBindPoint::eGraphics, pipeline);
        };
        auto operator=(const GraphicsPipeLine&) -> GraphicsPipeLine = delete;
        auto operator=(GraphicsPipeLine&&) -> GraphicsPipeLine& = default;
        auto operator()() -> ::vk::Pipeline& { return pipeline; }
        void destroy(::vk::Device& device);
        static void enableAlphaBlending(PipelineConfigInfo& configInfo);
        static auto getDefaultConfig() -> PipelineConfigInfo;

        ~GraphicsPipeLine() = default;
};

class ComputePipeline {
    private:
        ::vk::Pipeline pipeline_;
        ::vk::PipelineLayout pipelineLayout_;
        void createPipelineLayout(const ::vk::Device& device, const ::vk::DescriptorSetLayout& descriptorSetLayout);
        void createPipeline(const ::vk::Device& device,
                            const ::vk::PipelineShaderStageCreateInfo& shaderStageCreateInfo);

    public:
        void init(const ::vk::Device& device, const ::vk::PipelineShaderStageCreateInfo& shaderStageCreateInfo,
                  const ::vk::DescriptorSetLayout& descriptorSetLayout);
        void destroy(const ::vk::Device& device) const;
};

}  // namespace g
