#include "g_render_system.hpp"
#include "resource/shader.hpp"
#include "g_context.hpp"
#include "g_defines.hpp"
#include <spdlog/spdlog.h>
namespace g
{

void RenderSystem::render(FrameInfo& frameInfo)
{
    pipeline.bind(frameInfo.commandBuffer);
    frameInfo.commandBuffer.bindDescriptorSets(::vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, frameInfo.descriptorSet, nullptr);
    for(auto& kv :frameInfo.gameObjects)
    {
        auto& obj = kv.second;
        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }
}

    
RenderSystem::RenderSystem(::vk::RenderPass renderPass, ::vk::DescriptorSetLayout descriptorSetLayout)
{
    descriptor_.init(Context::Instance().device().logicalDevice());
    createPipelineLayout(descriptorSetLayout);
    createPipeline(renderPass);
}

RenderSystem::~RenderSystem()
{
    spdlog::debug(DETAIL_INFO("RenderSystem"));
    auto & device = Context::Instance().device().logicalDevice();
    device.destroyPipeline(pipeline());
    descriptor_.destory(device);
    device.destroyPipelineLayout(pipelineLayout);
}



void RenderSystem::createPipelineLayout(::vk::DescriptorSetLayout descriptorSetLayout)
{
    auto& device =  Context::Instance().device().logicalDevice();
    ::vk::PipelineLayoutCreateInfo layoutCreateInfo;
    
    layoutCreateInfo.setSetLayouts(descriptorSetLayout);
    
    //::vk::PushConstantRange pushConstantRange;
    // pushConstantRange.setStageFlags(::vk::ShaderStageFlagBits::eVertex | 
    //                             ::vk::ShaderStageFlagBits::eFragment)
    //                 .setOffset(0)
    //                 .setSize(sizeof(SimplePushConstantData));
    //layoutCreateInfo.setPushConstantRanges(pushConstantRange);  

    pipelineLayout = device.createPipelineLayout(layoutCreateInfo); 
}

void RenderSystem::createPipeline(::vk::RenderPass renderPass)
{
    ::std::string vertFilePath = shader_path + "vert.spv";
    ::std::string fragFilePath = shader_path + "frag.spv";
    ::resource::shader::Shader shader(vertFilePath, fragFilePath, Context::Instance().device().logicalDevice());
    auto defaultConfig= PipeLine::getDefaultConfig();
    defaultConfig.renderPass = renderPass;
    defaultConfig.layout = pipelineLayout;
    defaultConfig.shaderStages = shader.getShaderStages();
    defaultConfig.attributeDescriptions = Model::Vertex::getAtrributeDescription();
    defaultConfig.bindingDescriptions = Model::Vertex::getBindingDescription();
    defaultConfig.multisampleInfo.setRasterizationSamples(Context::Instance().imageQualityConfig.msaaSamples);
    pipeline.initPipeline(Context::Instance().device().logicalDevice(), defaultConfig);
}

}
