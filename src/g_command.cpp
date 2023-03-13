#include "g_command.hpp"
#include "g_device.hpp"
#include "g_swapchain.hpp"
#include "g_shader.hpp"
#include <exception>

namespace g
{

::std::unique_ptr<Command> Command::instance = nullptr;

void Command::init(int count)
{
    instance.reset(new Command(count));
}
void Command::quit()
{
    instance.reset();
}

void Command::runCmd(::vk::Pipeline pipeline, ::vk::RenderPass renderPass, int index, ::vk::Fence& fence, ::vk::Semaphore& semaphore)
{
    if (imagesInFlight[index] != nullptr) {
        auto result = Device::getInstance().getVKDevice().waitForFences(*imagesInFlight[index], true, ::std::numeric_limits<uint16_t>::max());
        if(result != ::vk::Result::eSuccess)
        {
            throw ::std::runtime_error("RenderProcess::render waitForFences error");
        }
    }
    imagesInFlight[index] = &fence;
    commandBuffers_[index].reset();
    ::vk::CommandBufferBeginInfo begin;
    ::vk::CommandBufferInheritanceInfo inherritanceInfo;
    begin.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        .setPInheritanceInfo(&inherritanceInfo);

    commandBuffers_[index].begin(begin);{
        commandBuffers_[index].bindPipeline(::vk::PipelineBindPoint::eGraphics, pipeline);
        ::vk::RenderPassBeginInfo renderPassBeginInfo;
        ::vk::Rect2D area;
        area.setOffset({0, 0})
            .setExtent(Swapchain::getInstance().getSwapchainInfo().extent2D);
        
        //下标0 颜色附件，下标1深度附件
        ::std::array<::vk::ClearValue, 2> clearValues;
        clearValues[0].setColor(::vk::ClearColorValue(::std::array<float, 4>{0.1f, 0.1f, 0.1f, 0.1f}));
        clearValues[1].setDepthStencil({1.0f, 0});

        renderPassBeginInfo.setRenderPass(renderPass)
                            .setRenderArea(area)
                            .setFramebuffer(Swapchain::getInstance().getFrameBuffer(index))
                            .setClearValues(clearValues);
        commandBuffers_[index].beginRenderPass(renderPassBeginInfo, {});{
            //commandBuffers_[index].draw(3, 1, 0, 0);
            Shader::getInstance().getModel().bind(commandBuffers_[index]);
            Shader::getInstance().getModel().draw(commandBuffers_[index]);
        }

        commandBuffers_[index].endRenderPass();
    }
    commandBuffers_[index].end();
    ::vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(commandBuffers_[index])
                .setPWaitSemaphores(&semaphore);
    Device::getInstance().getVKDevice().resetFences(fence);
    Device::getInstance().getGraphicsQueue().submit(submitInfo, fence);


    ::vk::PresentInfoKHR presentInfo;
    uint32_t imageIndex = (uint32_t)index;
    presentInfo.setImageIndices(imageIndex)
                .setPSwapchains(&Swapchain::getInstance().swapchain);

    auto result = Device::getInstance().getPresentQueue().presentKHR(presentInfo);
    if (result != ::vk::Result::eSuccess)
    {
        throw ::std::runtime_error("presentKHR");
    }
    if(result != ::vk::Result::eSuccess)
    {
        throw ::std::runtime_error("waitForFences");
    }
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
}

Command::~Command()
{
    auto& device = Device::getInstance().getVKDevice();
    
    for(auto& commandBuffer : commandBuffers_){
        device.freeCommandBuffers(cmdPool_, commandBuffer);
    }
    
    device.destroyCommandPool(cmdPool_);
}


}