#include "g_command.hpp"
#include "g_device.hpp"
#include "g_swapchain.hpp"
#include <exception>

namespace g
{

::std::unique_ptr<Command> Command::instance = nullptr;

void Command::init()
{
    instance.reset(new Command());
}
void Command::quit()
{
    instance.reset();
}

void Command::runCmd(::vk::Pipeline pipeline, ::vk::RenderPass renderPass, int index)
{
    cmdBuffer_.reset();
    ::vk::CommandBufferBeginInfo begin;
    ::vk::CommandBufferInheritanceInfo inherritanceInfo;
    begin.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        .setPInheritanceInfo(&inherritanceInfo);
    Device::getInstance().getVKDevice().resetFences(fence);

    cmdBuffer_.begin(begin);{
        cmdBuffer_.bindPipeline(::vk::PipelineBindPoint::eGraphics, pipeline);
        ::vk::RenderPassBeginInfo renderPassBeginInfo;
        ::vk::Rect2D area;
        ::vk::ClearValue clearValue;
        clearValue.color = ::vk::ClearColorValue(::std::array<float, 4>{0.1f, 0.1f, 0.1f, 0.1f});
        area.setOffset({0, 0})
            .setExtent(Swapchain::getInstance().getSwapchainInfo().extent2D);
        renderPassBeginInfo.setRenderPass(renderPass)
                            .setRenderArea(area)
                            .setFramebuffer(Swapchain::getInstance().getFrameBuffer(index))
                            .setClearValues(clearValue);
        cmdBuffer_.beginRenderPass(renderPassBeginInfo, {});{
            cmdBuffer_.draw(3, 1, 0, 0);
        }

        cmdBuffer_.endRenderPass();
    }
    cmdBuffer_.end();
    ::vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(cmdBuffer_);

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

    result = Device::getInstance().getVKDevice().waitForFences(fence, true, ::std::numeric_limits<uint16_t>::max());
    if(result != ::vk::Result::eSuccess)
    {
        throw ::std::runtime_error("waitForFences");
    }
}

void Command::createFance()
{
    ::vk::FenceCreateInfo info;
    //info.setFlags(::vk::FenceCreateFlagBits::eSignaled);
    fence = Device::getInstance().getVKDevice().createFence(info);
}

void Command::initCmdPool()
{
    ::vk::CommandPoolCreateInfo createInfo;
    createInfo.setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    cmdPool_ =  Device::getInstance().getVKDevice().createCommandPool(createInfo);
}
void Command::allcoCmdBuffer()
{
    ::vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandPool(cmdPool_)
        .setCommandBufferCount(1)
        .setLevel(::vk::CommandBufferLevel::ePrimary);
    cmdBuffer_ = Device::getInstance().getVKDevice().allocateCommandBuffers(allocInfo)[0];
}

Command::Command()
{
    initCmdPool();
    allcoCmdBuffer();
    createFance();
}
Command::~Command()
{
    auto device = Device::getInstance().getVKDevice();
    device.freeCommandBuffers(cmdPool_, cmdBuffer_);
    device.destroyCommandPool(cmdPool_);
     Device::getInstance().getVKDevice().destroyFence(fence);
}


}