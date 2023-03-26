#include "g_command.hpp"
#include "g_device.hpp"
#include "g_shader.hpp"
#include <exception>


namespace g
{

void Command::begin(int index)
{
    commandBuffers_[index].reset();
    ::vk::CommandBufferBeginInfo begin;
    ::vk::CommandBufferInheritanceInfo inherritanceInfo;
    begin.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        .setPInheritanceInfo(&inherritanceInfo);
    commandBuffers_[index].begin(begin);
}

void Command::beginRenderPass(int index, ::vk::RenderPass& renderPass, ::vk::Extent2D& extent, vk::Framebuffer& frameBuffer)
{    ::vk::RenderPassBeginInfo renderPassBeginInfo;
    ::vk::Rect2D area;
    area.setOffset({0, 0})
        .setExtent(extent);
    
    //下标0 颜色附件，下标1深度附件
    ::std::array<::vk::ClearValue, 2> clearValues;
    clearValues[0].setColor(::vk::ClearColorValue(std::array<float, 4>({0.01f, 0.01f, 0.01f, 1.0f})));
    clearValues[1].setDepthStencil({1.0f, 0});

    renderPassBeginInfo.setRenderPass(renderPass)
                        .setRenderArea(area)
                        .setFramebuffer(frameBuffer)
                        .setClearValues(clearValues);
    commandBuffers_[index].beginRenderPass(renderPassBeginInfo, ::vk::SubpassContents::eInline);

    ::vk::Viewport viewPort;
    viewPort.setX(0.0f)
            .setY(0.0f)
            .setWidth(static_cast<float>(extent.width))
            .setHeight(static_cast<float>(extent.height))
            .setMinDepth(.0f)
            .setMaxDepth(1.f);
    ::vk::Rect2D scissor{{0, 0}, extent};    
    commandBuffers_[index].setViewport(0, viewPort);
    commandBuffers_[index].setScissor(0, scissor);
}

void Command::endRenderPass(int index)
{
    commandBuffers_[index].endRenderPass();
}

::vk::Result Command::end(int bufferIndex, uint32_t imageIndex, ::vk::SwapchainKHR& swapchain, ::vk::Semaphore& waitSemaphore, ::vk::Fence& fence){
    commandBuffers_[bufferIndex].end();
    ::vk::SubmitInfo submitInfo;
    ::vk::PipelineStageFlags stage = ::vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.setCommandBuffers(commandBuffers_[bufferIndex])
                .setWaitSemaphores(waitSemaphore)
                .setSignalSemaphores(renderFinshSemaphores[bufferIndex])
                .setWaitDstStageMask(stage);
    Device::getInstance().getVKDevice().resetFences(fence);
    Device::getInstance().getGraphicsQueue().submit(submitInfo, fence);
    ::vk::PresentInfoKHR presentInfo;
    presentInfo.setImageIndices(imageIndex)
                .setSwapchains(swapchain)
                .setWaitSemaphores(renderFinshSemaphores[bufferIndex]);

    return Device::getInstance().getPresentQueue().presentKHR(presentInfo);
}

void Command::initCmdPool()
{
    ::vk::CommandPoolCreateInfo createInfo;
    uint32_t queueFamilyIndex = Device::getInstance().queueFamilyIndices.presentQueue.value();
    createInfo.setQueueFamilyIndex(queueFamilyIndex)
                .setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
                        ::vk::CommandPoolCreateFlagBits::eTransient);
    cmdPool_ =  Device::getInstance().getVKDevice().createCommandPool(createInfo);
}
void Command::allcoCmdBuffer(int count)
{
    ::vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandPool(cmdPool_)
        .setCommandBufferCount(count)
        .setLevel(::vk::CommandBufferLevel::ePrimary);
    
    commandBuffers_ = Device::getInstance().getVKDevice().allocateCommandBuffers(allocInfo);
}

Command::Command(int count)
{
    initCmdPool();
    allcoCmdBuffer(count);
    imagesInFlight.resize(count, nullptr);
    createSemaphore(count);
}

Command::~Command()
{
    auto& device = Device::getInstance().getVKDevice();
    
    for(auto & semaphore : renderFinshSemaphores){
        device.destroySemaphore(semaphore);
    }    
    device.destroyCommandPool(cmdPool_);
}

void Command::createSemaphore(int count)
{
    renderFinshSemaphores.resize(count);
    for(int i = 0; i < count; i++)
    {
        ::vk::SemaphoreCreateInfo semaphoreCreateInfo;
        renderFinshSemaphores[i] = Device::getInstance().getVKDevice().createSemaphore(semaphoreCreateInfo);
    }
}

}