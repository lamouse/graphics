#include "g_render_system.hpp"
#include "g_device.hpp"
#include "g_defines.hpp"
#include <memory>
#include <chrono>
namespace g
{

void RenderSystem::renderGameObject(::std::vector<GameObject>& gameObjects, int currentFrame, ::vk::CommandBuffer commandBuffer, float extentAspectRation)
{
    if(gameObjects.empty())
    {
        return;
    }
    static bool c = false;
    if(!c){
        gameObjects[0].loadImage();
        createDescriptorSets(2, gameObjects[0].textureImageView, gameObjects[0].textureSampler);
        c = true;
    }
    pipeline->bind(commandBuffer);
    updateUniformBuffer(currentFrame, extentAspectRation);
    for(auto& obj : gameObjects)
    {
        
        SimplePushConstantData push;
        push.color = obj.color;
        //obj.transform.rotation.y = ::glm::mod(obj.transform.rotation.y + 0.0003f, ::glm::two_pi<float>());
        //obj.transform.rotation.x = ::glm::mod(obj.transform.rotation.x + 0.0001f, ::glm::two_pi<float>());
        push.transform = obj.transform.mat4();
        // commandBuffer.pushConstants(pipelineLayout, ::vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 
        //                         0, sizeof(SimplePushConstantData), &push);
        obj.model->bind(commandBuffer);
        commandBuffer.bindDescriptorSets(::vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[currentFrame], nullptr);
        obj.model->draw(commandBuffer);
    }
}


void RenderSystem::createUniformBuffers(uint32_t count)
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    auto& device = Device::getInstance();
    uniformBuffers.resize(count);
    uniformBuffersMemory.resize(count);
    uniformBuffersMapped.resize(count);
    for (size_t i = 0; i < count; i++) {
        device.createBuffer(bufferSize, ::vk::BufferUsageFlagBits::eUniformBuffer, 
                    ::vk::MemoryPropertyFlagBits::eHostVisible | ::vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffers[i], uniformBuffersMemory[i]);

        uniformBuffersMapped[i] = device.getVKDevice().mapMemory(uniformBuffersMemory[i], 0, bufferSize);
    }

    createDescriptorPool(count);
}

void RenderSystem::createDescriptorPool(uint32_t count)
{

    std::array<::vk::DescriptorPoolSize, 2> poolSizes{};
    poolSizes[0] = ::vk::DescriptorPoolSize(::vk::DescriptorType::eUniformBuffer, count);
    poolSizes[1] = ::vk::DescriptorPoolSize(::vk::DescriptorType::eCombinedImageSampler, count);

    
    ::vk::DescriptorPoolCreateInfo createInfo;
    createInfo.setPoolSizes(poolSizes)
               .setMaxSets(count);
    descriptorPool = Device::getInstance().getVKDevice().createDescriptorPool(createInfo); 
}

void RenderSystem::createDescriptorSets(uint32_t count, ::vk::ImageView imageView, ::vk::Sampler sampler)
{
    ::std::vector<::vk::DescriptorSetLayout> layouts(count, setLayout);
    ::vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.setDescriptorPool(descriptorPool)
            .setSetLayouts(layouts);
    descriptorSets = Device::getInstance().getVKDevice().allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < count; i++) {
        ::vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        ::vk::DescriptorImageInfo imageInfo{};
        imageInfo.setImageLayout(::vk::ImageLayout::eShaderReadOnlyOptimal)
                    .setImageView(imageView)
                    .setSampler(sampler);
        
        ::vk::WriteDescriptorSet uniformDescriptorWrite{};
        uniformDescriptorWrite.setDstSet(descriptorSets[i])
                        .setDstBinding(0)
                        .setDstArrayElement(0)
                        .setDescriptorType(::vk::DescriptorType::eUniformBuffer)
                        .setBufferInfo(bufferInfo);
        ::vk::WriteDescriptorSet imageDescriptorWrite{};
        imageDescriptorWrite.setDstSet(descriptorSets[i])
                        .setDstBinding(1)
                        .setDstArrayElement(0)
                        .setDescriptorType(::vk::DescriptorType::eCombinedImageSampler)
                        .setImageInfo(imageInfo);

        std::array<::vk::WriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0] = uniformDescriptorWrite;
        descriptorWrites[1] = imageDescriptorWrite;
        Device::getInstance().getVKDevice().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
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
    auto & device = Device::getInstance().getVKDevice();
      for (size_t i = 0; i < uniformBuffers.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
    device.destroyDescriptorPool(descriptorPool);
    device.destroyDescriptorSetLayout(setLayout);
    device.destroyPipelineLayout(pipelineLayout);
}

void RenderSystem::createPipelineLayout()
{
    auto& device = Device::getInstance().getVKDevice();
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

    pipeline = ::std::make_unique<PipeLine>(shader_path + "vert.spv", shader_path + "frag.spv", renderPass, pipelineLayout);
}

}
