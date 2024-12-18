#include "g_pipeline.hpp"
#include "core/device.hpp"
#include <spdlog/spdlog.h>

namespace g {

GraphicsPipeLine::GraphicsPipeLine( PipelineConfigInfo& configInfo) {
    ::vk::GraphicsPipelineCreateInfo createInfo;

    ::vk::PipelineVertexInputStateCreateInfo inputState;
    inputState.setVertexBindingDescriptions(configInfo.bindingDescriptions);
    inputState.setVertexAttributeDescriptions(configInfo.attributeDescriptions);
    createInfo.setPVertexInputState(&inputState);
    createInfo.setStages(configInfo.shaderStages);

    createInfo.setPViewportState(&configInfo.viewportStateInfo)
        .setPDepthStencilState(&configInfo.depthStencilStateInfo)
        .setPRasterizationState(&configInfo.rasterizationStateInfo)
        .setPColorBlendState(&configInfo.colorBlendInfo)
        .setPInputAssemblyState(&configInfo.inputAssemblyStateInfo)
        .setPDynamicState(&configInfo.dynamicStateInfo)
        .setPMultisampleState(&configInfo.multisampleInfo)
        .setLayout(configInfo.layout)
        .setBasePipelineHandle(VK_NULL_HANDLE)
        .setBasePipelineIndex(-1)
        .setRenderPass(configInfo.renderPass)
        .setSubpass(0);
    core::Device device;

    const auto result = device.logicalDevice().createGraphicsPipeline(nullptr, createInfo);
    if (result.result != vk::Result::eSuccess) {
        throw ::std::runtime_error("create graphics pipeline failed");
    }

    pipeline = result.value;
}

void GraphicsPipeLine::enableAlphaBlending(PipelineConfigInfo& configInfo) {
    configInfo.colorBlendAttachmentInfo.setBlendEnable(VK_TRUE)
        .setColorWriteMask(::vk::ColorComponentFlagBits::eR | ::vk::ColorComponentFlagBits::eG |
                           ::vk::ColorComponentFlagBits::eB | ::vk::ColorComponentFlagBits::eA)
        .setSrcColorBlendFactor(::vk::BlendFactor::eSrc1Alpha)
        .setDstColorBlendFactor(::vk::BlendFactor::eOneMinusSrcAlpha)
        .setColorBlendOp(::vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(::vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(::vk::BlendOp::eAdd);
}

auto GraphicsPipeLine::getDefaultConfig() -> PipelineConfigInfo {
    PipelineConfigInfo configInfo;
    // dynamicState
    configInfo.dynamicStateEnables = {::vk::DynamicState::eViewport, ::vk::DynamicState::eScissor};
    configInfo.dynamicStateInfo.setDynamicStates(configInfo.dynamicStateEnables);

    // inputAssembly
    configInfo.inputAssemblyStateInfo.setTopology(::vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(VK_FALSE);

    // Viewport and Scissor
    configInfo.viewportStateInfo.setViewportCount(1).setScissorCount(1);

    configInfo.multisampleInfo.setSampleShadingEnable(false).setRasterizationSamples(::vk::SampleCountFlagBits::e1);

    configInfo.colorBlendAttachmentInfo.setBlendEnable(VK_FALSE).setColorWriteMask(
        ::vk::ColorComponentFlagBits::eR | ::vk::ColorComponentFlagBits::eG | ::vk::ColorComponentFlagBits::eB |
        ::vk::ColorComponentFlagBits::eA);
    configInfo.colorBlendInfo.setLogicOpEnable(false)
        .setLogicOp(vk::LogicOp::eCopy)
        .setAttachments(configInfo.colorBlendAttachmentInfo)
        .setBlendConstants({.0f, .0f, 0.f, .0f});

    configInfo.rasterizationStateInfo.setRasterizerDiscardEnable(VK_FALSE)
        .setDepthClampEnable(VK_FALSE)
        .setDepthBiasEnable(VK_FALSE)
        .setCullMode(::vk::CullModeFlagBits::eBack)
        .setFrontFace(::vk::FrontFace::eCounterClockwise)
        .setPolygonMode(::vk::PolygonMode::eFill)
        .setLineWidth(1.0f);

    configInfo.depthStencilStateInfo.setDepthTestEnable(VK_TRUE)
        .setDepthWriteEnable(VK_TRUE)
        .setDepthCompareOp(::vk::CompareOp::eLess)
        .setDepthBoundsTestEnable(VK_FALSE)
        .setMinDepthBounds(0.0f)
        .setMaxDepthBounds(1.0f)
        .setStencilTestEnable(VK_FALSE)
        .setFront({})
        .setBack({});

    return configInfo;
}

GraphicsPipeLine::GraphicsPipeLine(GraphicsPipeLine&& pipeline) noexcept : pipeline(pipeline.pipeline) {

    pipeline.pipeline = nullptr;
}
auto GraphicsPipeLine::operator=(GraphicsPipeLine&& rvalue)  noexcept -> GraphicsPipeLine&{
    this->pipeline = rvalue.pipeline;
    rvalue.pipeline = nullptr;
    return *this;
}
GraphicsPipeLine::~GraphicsPipeLine(){
    if (pipeline) {
        SPDLOG_DEBUG("destroy Graphics pipeline");
        core::Device device;
        device.logicalDevice().destroyPipeline(pipeline);
    }
}

void ComputePipeline::init( const vk::PipelineShaderStageCreateInfo& shaderStageCreateInfo,
                           const ::vk::DescriptorSetLayout& descriptorSetLayout) {
    createPipelineLayout(descriptorSetLayout);
    createPipeline(shaderStageCreateInfo);
}

void ComputePipeline::createPipelineLayout(const ::vk::DescriptorSetLayout& descriptorSetLayout) {
    ::vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setSetLayouts(descriptorSetLayout);
    core::Device device;
    pipelineLayout_ = device.logicalDevice().createPipelineLayout(pipelineLayoutInfo, nullptr);
}

void ComputePipeline::destroy() const {
    core::Device device;
    device.logicalDevice().destroyPipeline(pipeline_);
    device.logicalDevice().destroyPipelineLayout(pipelineLayout_);
}

void ComputePipeline::createPipeline(const ::vk::PipelineShaderStageCreateInfo& shaderStageCreateInfo) {
    ::vk::ComputePipelineCreateInfo createInfo;
    createInfo.setStage(shaderStageCreateInfo);
    createInfo.setLayout(pipelineLayout_);
    core::Device device;
    const auto result = device.logicalDevice().createComputePipelines(nullptr, createInfo);
    if (result.result != ::vk::Result::eSuccess) {
        throw ::std::runtime_error("create compute pipeline failed");
    }
    pipeline_ = result.value[0];
}

}  // namespace g
