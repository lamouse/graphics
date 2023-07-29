#pragma once

#include <memory>
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
class PipeLine {
    private:
        ::vk::Pipeline pipeline;

    public:
        PipeLine() = default;
        void initPipeline(::vk::Device& device, PipelineConfigInfo& configInfo);
        PipeLine(const PipeLine&) = delete;
        void bind(::vk::CommandBuffer& commandBuffer) {
            commandBuffer.bindPipeline(::vk::PipelineBindPoint::eGraphics, pipeline);
        };
        auto operator=(const PipeLine&) -> PipeLine = delete;
        auto operator=(PipeLine&&) -> PipeLine& = default;
        auto operator()() -> ::vk::Pipeline& { return pipeline; }
        void destroy(::vk::Device& device);
        static void enableAlphaBlending(PipelineConfigInfo& configInfo);
        static auto getDefaultConfig() -> PipelineConfigInfo;

        ~PipeLine() = default;
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
