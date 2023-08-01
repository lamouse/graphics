#include "g_render_system.hpp"

#include <spdlog/spdlog.h>

#include "g_context.hpp"
#include "g_defines.hpp"
#include "g_model.hpp"
#include "resource/shader.hpp"

namespace g {

void RenderSystem::render(FrameInfo& frameInfo) {
    pipeline.bind(frameInfo.commandBuffer);
    frameInfo.commandBuffer.bindDescriptorSets(::vk::PipelineBindPoint::eGraphics, pipelineLayout, 0,
                                               frameInfo.descriptorSet, nullptr);
    for (auto& [k, v] : frameInfo.gameObjects) {
        auto& obj = v;
        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }
}

RenderSystem::RenderSystem(::vk::RenderPass renderPass, ::vk::DescriptorSetLayout descriptorSetLayout) {
    createPipelineLayout(descriptorSetLayout);
    createPipeline(renderPass);
}

RenderSystem::~RenderSystem() {
    spdlog::debug(DETAIL_INFO("RenderSystem"));
    auto& device = Context::Instance().device().logicalDevice();
    device.destroyPipeline(pipeline());
    device.destroyPipelineLayout(pipelineLayout);
}

void RenderSystem::createPipelineLayout(::vk::DescriptorSetLayout descriptorSetLayout) {
    auto& device = Context::Instance().device().logicalDevice();
    ::vk::PipelineLayoutCreateInfo layoutCreateInfo;

    layoutCreateInfo.setSetLayouts(descriptorSetLayout);

    //::vk::PushConstantRange pushConstantRange;
    // pushConstantRange.setStageFlags(::vk::ShaderStageFlagBits::eVertex |
    //                             ::vk::ShaderStageFlagBits::eFragment)
    //                 .setOffset(0)
    //                 .setSize(sizeof(SimplePushConstantData));
    // layoutCreateInfo.setPushConstantRanges(pushConstantRange);

    pipelineLayout = device.createPipelineLayout(layoutCreateInfo);
}

void RenderSystem::createPipeline(::vk::RenderPass renderPass) {
    ::std::string vertFilePath = shader_path + "vert.spv";
    ::std::string fragFilePath = shader_path + "frag.spv";
    ::resource::shader::GraphicsShader shader(vertFilePath, fragFilePath, Context::Instance().device().logicalDevice());
    auto defaultConfig = GraphicsPipeLine::getDefaultConfig();
    defaultConfig.renderPass = renderPass;
    defaultConfig.layout = pipelineLayout;
    defaultConfig.shaderStages = shader.getShaderStages();
    defaultConfig.attributeDescriptions = Model::Vertex::getAttributeDescription();
    defaultConfig.bindingDescriptions = Model::Vertex::getBindingDescription();
    defaultConfig.multisampleInfo.setRasterizationSamples(Context::Instance().imageQualityConfig.msaaSamples);
    pipeline.initPipeline(Context::Instance().device().logicalDevice(), defaultConfig);
}

}  // namespace g
