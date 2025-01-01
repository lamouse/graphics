#include "g_render_system.hpp"

#include <spdlog/spdlog.h>

#include <ranges>

#include "g_defines.hpp"
#include "g_model.hpp"
#include "resource/shader.hpp"

namespace g {

void RenderSystem::render(FrameInfo& frameInfo) const {
    pipeline.bind(frameInfo.commandBuffer);
    frameInfo.commandBuffer.bindDescriptorSets(::vk::PipelineBindPoint::eGraphics, pipelineLayout,
                                               0, frameInfo.descriptorSet, nullptr);

    for (const auto& v : frameInfo.gameObjects | std::views::values) {
        v.model->bind(frameInfo.commandBuffer);
        v.model->draw(frameInfo.commandBuffer);
    }
}

RenderSystem::RenderSystem(::core::Device& device, ::vk::RenderPass renderPass,
                           ::vk::DescriptorSetLayout descriptorSetLayout)
    : device_(device) {
    createPipelineLayout(descriptorSetLayout);
    createPipeline(renderPass);
}

RenderSystem::~RenderSystem() {
    spdlog::debug(DETAIL_INFO("RenderSystem"));
    const auto& device = device_.logicalDevice();
    device.destroyPipelineLayout(pipelineLayout);
}

void RenderSystem::createPipelineLayout(::vk::DescriptorSetLayout descriptorSetLayout) {
    const auto& device = device_.logicalDevice();
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
    const ::std::string vertexFilePath = shader_path + "vulkan_present.spv";
    const ::std::string fragFilePath = shader_path + "vulkan_present.spv";
    ::resource::shader::GraphicsShader shader(vertexFilePath, fragFilePath,
                                              device_.logicalDevice());
    auto defaultConfig = GraphicsPipeLine::getDefaultConfig();
    defaultConfig.renderPass = renderPass;
    defaultConfig.layout = pipelineLayout;
    defaultConfig.shaderStages = shader.getShaderStages();
    defaultConfig.attributeDescriptions = Model::Vertex::getAttributeDescription();
    defaultConfig.bindingDescriptions = Model::Vertex::getBindingDescription();
    defaultConfig.multisampleInfo.setRasterizationSamples(device_.getMaxMsaaSamples());
    pipeline = GraphicsPipeLine(defaultConfig);
}

}  // namespace g
