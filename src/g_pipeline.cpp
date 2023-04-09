#include "g_pipeline.hpp"
#include "g_device.hpp"
#include "g_swapchain.hpp"
#include "g_model.hpp"
#include "resource/shader.hpp"
#include <iostream>


namespace g{
PipeLine::PipeLine(const ::std::string& vertFilePath, const ::std::string& fragFilePath, ::vk::RenderPass& renderPass, ::vk::PipelineLayout layout)
{
    currentFrame = 0;
    initPipeline(vertFilePath, fragFilePath, renderPass, layout);
}

PipeLine::~PipeLine()
{

    Device::getInstance().getVKDevice().destroyPipeline(pipeline);
}

void PipeLine::initPipeline(const ::std::string& vertFilePath, const ::std::string& fragFilePath, ::vk::RenderPass& renderPass, ::vk::PipelineLayout layout)
{
    defaultPiplineConfig();
    ::vk::GraphicsPipelineCreateInfo createInfo;
    
    ::vk::PipelineVertexInputStateCreateInfo inputState;
    inputState.setVertexBindingDescriptions(configInfo.bindingDescriptions);
    inputState.setVertexAttributeDescriptions(configInfo.attributeDescriptions);
    createInfo.setPVertexInputState(&inputState);  
    auto& device = Device::getInstance().getVKDevice();
    ::resource::shader::Shader shader(vertFilePath, fragFilePath, device);
    auto shaderStage = shader.getShaderStage();
    createInfo.setStages(shaderStage);
 
    createInfo.setPViewportState(&configInfo.viewportStateInfo)
                .setPDepthStencilState(&configInfo.depthStencilStateInfo)
                .setPRasterizationState(&configInfo.rasterizationStateInfo)
                .setPColorBlendState(&configInfo.colorBlendInfo)
                .setPInputAssemblyState(&configInfo.inputAssemblyStateInfo)
                .setPDynamicState(&configInfo.dynamicStateInfo)
                .setPMultisampleState(&configInfo.multisampleInfo)
                .setLayout(layout)
                .setBasePipelineHandle(VK_NULL_HANDLE)
                .setBasePipelineIndex(-1)
                .setRenderPass(renderPass)
                .setSubpass(0);
    auto result = Device::getInstance().getVKDevice().createGraphicsPipeline(nullptr, createInfo);
    if(result.result != vk::Result::eSuccess)
    {
        throw ::std::runtime_error("create graphics pipeline failed");
    }

    pipeline = result.value;
}

void PipeLine::defaultPiplineConfig()
{
    setDepthStencilStateConfig();
    setRasterizationStateInfo();
    setColorBlendInfo();
    setMultisampleInfo();
    setViewportStateInfo();
    setInputAssemblyStateInfo();
    setDynamicStateEnables();

    configInfo.attributeDescriptions = Model::Vertex::getAtrributeDescription();
    configInfo.bindingDescriptions = Model::Vertex::getBindingDescription();
}

void PipeLine::setDepthStencilStateConfig()
{
    configInfo.depthStencilStateInfo.setDepthTestEnable(VK_TRUE)
                                    .setDepthWriteEnable(VK_TRUE)
                                    .setDepthCompareOp(::vk::CompareOp::eLess)
                                    .setDepthBoundsTestEnable(VK_FALSE)
                                    .setMinDepthBounds(0.1f)
                                    .setMaxDepthBounds(1.0f)
                                    .setStencilTestEnable(VK_FALSE)
                                    .setFront({})
                                    .setBack({});
}

void PipeLine::setRasterizationStateInfo()
{
    configInfo.rasterizationStateInfo.setRasterizerDiscardEnable(false)
                                    .setDepthClampEnable(VK_FALSE)
                                    .setCullMode(::vk::CullModeFlagBits::eBack)
                                    .setFrontFace(::vk::FrontFace::eCounterClockwise)
                                    .setPolygonMode(::vk::PolygonMode::eFill)
                                    .setLineWidth(1);
}

void PipeLine::setColorBlendInfo()
{
     
    configInfo.colorBlendAttachsInfo.setBlendEnable(false)
                                    .setColorWriteMask( ::vk::ColorComponentFlagBits::eR | 
                                                        ::vk::ColorComponentFlagBits::eG |
                                                        ::vk::ColorComponentFlagBits::eB |
                                                        ::vk::ColorComponentFlagBits::eA );
    configInfo.colorBlendInfo.setLogicOpEnable(false)
                            .setAttachments(configInfo.colorBlendAttachsInfo);
}

void PipeLine::setMultisampleInfo()
{
    configInfo.multisampleInfo.setSampleShadingEnable(false)
                              .setRasterizationSamples(Device::getInstance().getMaxUsableSampleCount());
}

void PipeLine::setViewportStateInfo()
{
    configInfo.viewportStateInfo.setViewportCount(1)
                                .setScissorCount(1);
}

void PipeLine::setInputAssemblyStateInfo()
{
    configInfo.inputAssemblyStateInfo.setTopology(::vk::PrimitiveTopology::eTriangleList)
                                     .setPrimitiveRestartEnable(VK_FALSE);
}

void PipeLine::setDynamicStateEnables()
{
    configInfo.dynamicStateEnables = {::vk::DynamicState::eViewport, ::vk::DynamicState::eScissor};
    configInfo.dynamicStateInfo.setDynamicStates(configInfo.dynamicStateEnables);
}

void PipeLine::enableAlphaBlending(PipelineConfigInfo& configInfo) {
  configInfo.colorBlendAttachsInfo.setBlendEnable(VK_TRUE)
                                    .setColorWriteMask( ::vk::ColorComponentFlagBits::eR | 
                                                        ::vk::ColorComponentFlagBits::eG |
                                                        ::vk::ColorComponentFlagBits::eB |
                                                        ::vk::ColorComponentFlagBits::eA )
                                    .setSrcColorBlendFactor(::vk::BlendFactor::eSrc1Alpha)
                                    .setDstColorBlendFactor(::vk::BlendFactor::eOneMinusSrcAlpha)
                                    .setColorBlendOp(::vk::BlendOp::eAdd)
                                    .setSrcAlphaBlendFactor(::vk::BlendFactor::eOne)
                                    .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                                    .setAlphaBlendOp(::vk::BlendOp::eAdd);
}

}