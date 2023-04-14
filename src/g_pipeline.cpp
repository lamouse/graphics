#include "g_pipeline.hpp"
#include "g_model.hpp"

namespace g{

PipeLine::~PipeLine()
{

}

void PipeLine::initPipeline(::vk::Device& device, PipelineConfigInfo& configInfo)
{
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
    auto result = device.createGraphicsPipeline(nullptr, createInfo);
    if(result.result != vk::Result::eSuccess)
    {
        throw ::std::runtime_error("create graphics pipeline failed");
    }

    pipeline = result.value;
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

PipelineConfigInfo PipeLine::getDefaultConfig()
{
    PipelineConfigInfo configInfo;
    // dynamicState
    configInfo.dynamicStateEnables = {::vk::DynamicState::eViewport, ::vk::DynamicState::eScissor};
    configInfo.dynamicStateInfo.setDynamicStates(configInfo.dynamicStateEnables);

    //inputAssembly
    configInfo.inputAssemblyStateInfo.setTopology(::vk::PrimitiveTopology::eTriangleList)
                                     .setPrimitiveRestartEnable(VK_FALSE);
    
    // Viewport and Scissor
    configInfo.viewportStateInfo.setViewportCount(1)
                                .setScissorCount(1);

    configInfo.multisampleInfo.setSampleShadingEnable(false)
                              .setRasterizationSamples(::vk::SampleCountFlagBits::e1);


    configInfo.colorBlendAttachsInfo.setBlendEnable(false)
                                    .setColorWriteMask( ::vk::ColorComponentFlagBits::eR | 
                                                        ::vk::ColorComponentFlagBits::eG |
                                                        ::vk::ColorComponentFlagBits::eB |
                                                        ::vk::ColorComponentFlagBits::eA );
    configInfo.colorBlendInfo.setLogicOpEnable(false)
                            .setLogicOp(vk::LogicOp::eCopy)
                            .setAttachments(configInfo.colorBlendAttachsInfo)
                            .setBlendConstants({.0f, .0f, 0.f, .0f});


    configInfo.rasterizationStateInfo.setRasterizerDiscardEnable(VK_FALSE)
                                    .setDepthClampEnable(VK_FALSE)
                                    .setDepthClampEnable(VK_FALSE)
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

void PipeLine::destroy(::vk::Device& device)
{
    device.destroyPipeline(pipeline);
}

}