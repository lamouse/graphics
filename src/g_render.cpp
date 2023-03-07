#include "g_render.hpp"
#include "g_shader.hpp"
#include "g_device.hpp"

namespace g{
RenderProcess::RenderProcess(int width, int height)
{
    initPipeline(width, height);
}

RenderProcess::~RenderProcess()
{
}

void RenderProcess::initPipeline(int width, int height)
{
    ::vk::GraphicsPipelineCreateInfo createInfo;
    ::vk::PipelineVertexInputStateCreateInfo inputState;
    createInfo.setPVertexInputState(&inputState);
    ::vk::PipelineInputAssemblyStateCreateInfo inputAsm;
    inputAsm.setPrimitiveRestartEnable(false)
            .setTopology(::vk::PrimitiveTopology::eTriangleList);
    createInfo.setPInputAssemblyState(&inputAsm);
    auto shaderStage = Shader::getInstance().getShaderStage();
    createInfo.setStages(shaderStage);
    ::vk::PipelineViewportStateCreateInfo pipelineViewportState;
   
    ::vk::Viewport viewport(0, 0, width, height, -1, 1);
    
    pipelineViewportState.setPViewports(&viewport);
    createInfo.setPViewportState(&pipelineViewportState);
    ::vk::Rect2D rect({0, 0}, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
    pipelineViewportState.setScissors(rect);

    ::vk::PipelineRasterizationStateCreateInfo rastCreateInfo;
    rastCreateInfo.setRasterizerDiscardEnable(false)
                    .setCullMode(::vk::CullModeFlagBits::eBack)
                    .setFrontFace(::vk::FrontFace::eCounterClockwise)
                    .setPolygonMode(::vk::PolygonMode::eFill)
                    .setLineWidth(1);
    createInfo.setPRasterizationState(&rastCreateInfo);

    ::vk::PipelineMultisampleStateCreateInfo multisample;
    multisample.setSampleShadingEnable(false)
                .setRasterizationSamples(::vk::SampleCountFlagBits::e1);

    createInfo.setPMultisampleState(&multisample);

    ::vk::PipelineColorBlendStateCreateInfo blend;
    ::vk::PipelineColorBlendAttachmentState attachs;
    attachs.setBlendEnable(false)
            .setColorWriteMask(::vk::ColorComponentFlagBits::eA | 
                                            ::vk::ColorComponentFlagBits::eB |
                                            ::vk::ColorComponentFlagBits::eG |
                                            ::vk::ColorComponentFlagBits::eR );
    blend.setLogicOpEnable(false)
            .setAttachments(attachs);
    createInfo.setPColorBlendState(&blend);

    auto result = Device::getInstance().getVKDevice().createGraphicsPipeline(nullptr, createInfo);
    if(result.result != vk::Result::eSuccess)
    {
        throw ::std::runtime_error("create graphics pipeline failed");
    }

    pipline = result.value;
}

}