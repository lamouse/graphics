#include "g_render.hpp"
#include "g_shader.hpp""

namespace g{
RenderProcess::RenderProcess()
{

}

RenderProcess::~RenderProcess()
{
}

void RenderProcess::initPipeline()
{
    ::vk::GraphicsPipelineCreateInfo createInfo;
    ::vk::PipelineVertexInputStateCreateInfo inputState;
    createInfo.setPVertexInputState(&inputState);
    ::vk::PipelineInputAssemblyStateCreateInfo inputAsm;
    inputAsm.setPrimitiveRestartEnable(false)
            .setTopology(::vk::PrimitiveTopology::eTriangleList);
    createInfo.setPInputAssemblyState(&inputAsm);
    auto shaderStage = Shader::getInstance().getShaderStage();
    //createInfo.setPStages(&shaderStage);
}

}