#include "g_compute.hpp"
#include "resource/shader.hpp"
#include <limits>
namespace g {

Compute::Compute(::core::Device& device, ::std::string& path, ::vk::DescriptorSetLayout descriptorSetLayout):device_(device)
{
    createComputePipelineLayout(descriptorSetLayout);
    createComputePipeline(path);
}

void Compute::createComputePipeline(::std::string& path)
{
    resource::shader::ComputeShader computeShader(device_.logicalDevice(), path);
    ::vk::ComputePipelineCreateInfo createInfo;
    createInfo.setStage(computeShader.getShaderStages());
    createInfo.setLayout(computePipelineLayout_);
    auto result = device_.logicalDevice().createComputePipelines(nullptr ,createInfo);
    if (result.result != ::vk::Result::eSuccess)
    {
        throw ::std::runtime_error("create compute pipeline failed");
    }
    computePipeline_ = result.value[0];
}

void Compute::createComputePipelineLayout(::vk::DescriptorSetLayout descriptorSetLayout)
{
    ::vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setSetLayouts(descriptorSetLayout);
    computePipelineLayout_ = device_.logicalDevice().createPipelineLayout(pipelineLayoutInfo, nullptr);
}

void Compute::init(int inFlightCount)
{
    createCommandBuffers(inFlightCount);
    createSyncs(inFlightCount);
}

void Compute::createCommandBuffers(int inFlightCount)
{
    ::vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandPool(device_.getCommandPool())
        .setCommandBufferCount(inFlightCount)
        .setLevel(::vk::CommandBufferLevel::ePrimary);
    
    commandBuffers_ = device_.logicalDevice().allocateCommandBuffers(allocInfo);
}

void Compute::createSyncs(int inFlightCount)
{
    finishedSemaphores_.resize(inFlightCount);
    inFlightFences_.resize(inFlightCount);
    for(int i = 0; i < inFlightCount; ++i)
    {
        ::vk::SemaphoreCreateInfo semaphoreCreateInfo;
        ::vk::FenceCreateInfo fenceCreateInfo(::vk::FenceCreateFlagBits::eSignaled);
        finishedSemaphores_[i] = device_.logicalDevice().createSemaphore(semaphoreCreateInfo);
        inFlightFences_[i] = device_.logicalDevice().createFence(fenceCreateInfo);
    }
}

Compute::~Compute()
{
    for(int i = 0; i < finishedSemaphores_.size(); ++i)
    {
        device_.logicalDevice().destroyFence(inFlightFences_[i]);
        device_.logicalDevice().destroySemaphore(finishedSemaphores_[i]);
    }
    device_.logicalDevice().destroyPipeline(computePipeline_);
    device_.logicalDevice().destroyPipelineLayout(computePipelineLayout_);
}

void Compute::beginCompute(int currentFrameIndex)
{
    auto waitResult = device_.logicalDevice().waitForFences(inFlightFences_[currentFrameIndex], true, ::std::numeric_limits<uint64_t>::max());
    if(waitResult != ::vk::Result::eSuccess)
    {
        throw ::std::runtime_error(" Swapchain::acquireNextImage wait fences");
    }
    device_.logicalDevice().resetFences(inFlightFences_[currentFrameIndex]);
    commandBuffers_[currentFrameIndex].reset();
}
auto Compute::compute(int currentFrameIndex, ::vk::DescriptorSet descriptorSet) -> ::vk::CommandBuffer&
{
    ::vk::CommandBufferBeginInfo beginInfo{};
    commandBuffers_[currentFrameIndex].begin(beginInfo);
    commandBuffers_[currentFrameIndex].bindPipeline(vk::PipelineBindPoint::eCompute,computePipeline_);
    commandBuffers_[currentFrameIndex].bindDescriptorSets(vk::PipelineBindPoint::eCompute, computePipelineLayout_, 0, descriptorSet, nullptr);

    return commandBuffers_[currentFrameIndex];
}

void Compute::endCompute(int currentFrameIndex)
{
    commandBuffers_[currentFrameIndex].end();
    ::vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(commandBuffers_[currentFrameIndex])
                .setSignalSemaphores(finishedSemaphores_[currentFrameIndex]);
    device_.getComputeQueue().submit(submitInfo);

}

}