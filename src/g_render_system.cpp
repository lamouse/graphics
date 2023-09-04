#include "g_render_system.hpp"

#include <spdlog/spdlog.h>
#include <ranges>

#include "g_context.hpp"
#include "g_defines.hpp"
#include "g_model.hpp"
#include "resource/shader.hpp"

namespace g {

void RenderSystem::render(FrameInfo& frameInfo) const {
    pipeline.bind(frameInfo.commandBuffer);
    frameInfo.commandBuffer.bindDescriptorSets(::vk::PipelineBindPoint::eGraphics, pipelineLayout, 0,
                                               frameInfo.descriptorSet, nullptr);

    for (const auto& v : frameInfo.gameObjects | std::views::values) {
        v.model->bind(frameInfo.commandBuffer);
        v.model->draw(frameInfo.commandBuffer);
    }
}

RenderSystem::RenderSystem(::vk::RenderPass renderPass, ::vk::DescriptorSetLayout descriptorSetLayout) {
    createPipelineLayout(descriptorSetLayout);
    createPipeline(renderPass);
}

RenderSystem::~RenderSystem() {
    spdlog::debug(DETAIL_INFO("RenderSystem"));
    const auto& device = Context::Instance().device().logicalDevice();
    device.destroyPipeline(pipeline());
    device.destroyPipelineLayout(pipelineLayout);
}

void RenderSystem::createPipelineLayout(::vk::DescriptorSetLayout descriptorSetLayout) {
    const auto& device = Context::Instance().device().logicalDevice();
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
    const ::std::string vertexFilePath = shader_path + "vert.spv";
    const ::std::string fragFilePath = shader_path + "frag.spv";
    ::resource::shader::GraphicsShader shader(vertexFilePath, fragFilePath,
                                              Context::Instance().device().logicalDevice());
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
