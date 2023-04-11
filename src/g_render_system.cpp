#include "g_render_system.hpp"
#include "resource/shader.hpp"
#include "g_context.hpp"
#include "g_defines.hpp"
#include <chrono>
namespace g
{

void RenderSystem::renderGameObject(::std::vector<GameObject>& gameObjects, int currentFrame, ::vk::CommandBuffer commandBuffer, float extentAspectRation,
                                    resource::image::ImageTexture& imageTexture)
{
    if(gameObjects.empty())
    {
        return;
    }
    updateDescriptorSet(currentFrame, imageTexture.imageView(), imageTexture.sampler());
    pipeline.bind(commandBuffer);
    updateUniformBuffer(currentFrame, extentAspectRation);
    for(auto& obj : gameObjects)
    {
        obj.model->bind(commandBuffer);
        commandBuffer.bindDescriptorSets(::vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[currentFrame], nullptr);
        obj.model->draw(commandBuffer);
    }
}


void RenderSystem::createUniformBuffers(uint32_t count)
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    auto& device = Context::Instance().device();
    uniformBuffers.resize(count);
    uniformBuffersMemory.resize(count);
    uniformBuffersMapped.resize(count);
    for (size_t i = 0; i < count; i++) {
        device.createBuffer(bufferSize, ::vk::BufferUsageFlagBits::eUniformBuffer, 
                    ::vk::MemoryPropertyFlagBits::eHostVisible | ::vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffers[i], uniformBuffersMemory[i]);

        uniformBuffersMapped[i] = device.logicalDevice().mapMemory(uniformBuffersMemory[i], 0, bufferSize);
    }

    createDescriptorPool(count);
    createDescriptorSets(count);
}

void RenderSystem::createDescriptorPool(uint32_t count)
{
    count = 1000;
    std::array<::vk::DescriptorPoolSize, 11> poolSizes{
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eCombinedImageSampler, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eSampler, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eSampledImage, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageImage, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eUniformTexelBuffer, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageTexelBuffer, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eUniformBuffer, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageBuffer, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eUniformBufferDynamic, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageBufferDynamic, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eInputAttachment, count)
    };
    
    ::vk::DescriptorPoolCreateInfo createInfo;
    createInfo.setPoolSizes(poolSizes)
               .setMaxSets(count);
    descriptorPool =  Context::Instance().device().logicalDevice().createDescriptorPool(createInfo); 
}

void RenderSystem::createDescriptorSets(uint32_t count)
{
    ::std::vector<::vk::DescriptorSetLayout> layouts(count, setLayout);
    ::vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.setDescriptorPool(descriptorPool)
            .setSetLayouts(layouts);
    descriptorSets =  Context::Instance().device().logicalDevice().allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < count; i++) {
     
    }
}

void RenderSystem::updateDescriptorSet(uint32_t currentImage, ::vk::ImageView imageView, ::vk::Sampler sampler)
{
       ::vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[currentImage];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        ::vk::DescriptorImageInfo imageInfo{};
        imageInfo.setImageLayout(::vk::ImageLayout::eShaderReadOnlyOptimal)
                    .setImageView(imageView)
                    .setSampler(sampler);
        
        ::vk::WriteDescriptorSet uniformDescriptorWrite{};
        uniformDescriptorWrite.setDstSet(descriptorSets[currentImage])
                        .setDstBinding(0)
                        .setDstArrayElement(0)
                        .setDescriptorType(::vk::DescriptorType::eUniformBuffer)
                        .setBufferInfo(bufferInfo);
        ::vk::WriteDescriptorSet imageDescriptorWrite{};
        imageDescriptorWrite.setDstSet(descriptorSets[currentImage])
                        .setDstBinding(1)
                        .setDstArrayElement(0)
                        .setDescriptorType(::vk::DescriptorType::eCombinedImageSampler)
                        .setImageInfo(imageInfo);

        std::array<::vk::WriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0] = uniformDescriptorWrite;
        descriptorWrites[1] = imageDescriptorWrite;
        Context::Instance().device().logicalDevice().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void RenderSystem::updateUniformBuffer(uint32_t currentImage, float extentAspectRation)
{
    static auto startTime = ::std::chrono::high_resolution_clock::now();
    auto currentTime = ::std::chrono::high_resolution_clock::now();
    float time = ::std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    UniformBufferObject ubo{};
    ubo.model = ::glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = ::glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = ::glm::perspective(glm::radians(45.0f), extentAspectRation, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
    
RenderSystem::RenderSystem(::vk::RenderPass renderPass)
{
    createPipelineLayout();
    createPipeline(renderPass);
}

RenderSystem::~RenderSystem()
{
    auto & device = Context::Instance().device().logicalDevice();
    for (size_t i = 0; i < uniformBuffers.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    device.destroyPipeline(pipeline());
    device.destroyDescriptorPool(descriptorPool);
    device.destroyDescriptorSetLayout(setLayout);
    device.destroyPipelineLayout(pipelineLayout);
}

void RenderSystem::createPipelineLayout()
{
    auto& device =  Context::Instance().device().logicalDevice();
    ::vk::PushConstantRange pushConstantRange;
    pushConstantRange.setStageFlags(::vk::ShaderStageFlagBits::eVertex | 
                                    ::vk::ShaderStageFlagBits::eFragment)
                        .setOffset(0)
                        .setSize(sizeof(SimplePushConstantData));
    ::vk::PipelineLayoutCreateInfo layoutCreateInfo;
    
    ::vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.setBinding(0)
                    .setDescriptorType(::vk::DescriptorType::eUniformBuffer)
                    .setDescriptorCount(1)
                    .setStageFlags(::vk::ShaderStageFlagBits::eVertex);
    
    ::vk::DescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.setBinding(1)
                        .setDescriptorCount(1)
                        .setDescriptorType(::vk::DescriptorType::eCombinedImageSampler)
                        .setStageFlags(::vk::ShaderStageFlagBits::eFragment);

    std::array<::vk::DescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

    ::vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
    setLayoutCreateInfo.setBindings(bindings);
    setLayout = device.createDescriptorSetLayout(setLayoutCreateInfo);
    layoutCreateInfo.setSetLayouts(setLayout);

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
