#include "g_render.hpp"
#include "g_context.hpp"
#include <cassert>
#include <exception>
namespace g{
RenderProcesser::RenderProcesser(core::Device& device):device_(device)
{
    createSwapchain();
    allcoCmdBuffer();
}

RenderProcesser::~RenderProcesser()
{
}

void RenderProcesser::createSwapchain()
{
    auto extent = Context::getExtent();
    while (extent.width == 0 || extent.height == 0)
    {
        extent = Context::getExtent();  
        Context::waitWindowEvents();
    }
    device_.logicalDevice().waitIdle();
    if(swapchain == nullptr)
    {
        swapchain = ::std::make_unique<Swapchain>(device_, extent.width, extent.height);
    }else{
        ::std::shared_ptr<Swapchain> old = ::std::move(swapchain);
        swapchain = ::std::make_unique<Swapchain>(device_, extent.width, extent.height, old);
        if(!old->compareFormats(*swapchain)){
            throw ::std::runtime_error("swapchain image(or depth) format has changed!");
        }
    }
}

bool RenderProcesser::beginFrame()
{
    auto result  = swapchain->acquireNextImage();

    if(result.result == ::vk::Result::eErrorOutOfDateKHR)
    {
        createSwapchain();
        return isFrameStart;
    }
    if(result.result != ::vk::Result::eSuccess && result.result != ::vk::Result::eSuboptimalKHR)
    {
        throw ::std::runtime_error("faile acquire swapchain image");
    }

    isFrameStart = true;
    currentImageIndex = result.value;
    getCurrentCommadBuffer().reset();
    ::vk::CommandBufferBeginInfo begin;
    ::vk::CommandBufferInheritanceInfo inherritanceInfo;
    begin.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        .setPInheritanceInfo(&inherritanceInfo);
    getCurrentCommadBuffer().begin(begin);
    return isFrameStart;
}

void RenderProcesser::endFrame()
{
    assert(isFrameStart && "cat't call begin swapchin renderpass is frame not in progress");
    try
    {
        auto result = swapchain->submitCommand(getCurrentCommadBuffer(), currentImageIndex);

        if(result == ::vk::Result::eErrorOutOfDateKHR || result == ::vk::Result::eSuboptimalKHR || Context::isWindowRsize())
        {
            createSwapchain();
            Context::rsetWindowRsize();
        }else if (result != ::vk::Result::eSuccess)
        {
            throw new ::std::runtime_error("faild to present swap chain image");
        }
    }
    catch(const ::vk::OutOfDateKHRError& e)
    {
        createSwapchain();
        Context::rsetWindowRsize();
    }

    isFrameStart = false;
    currentFrameIndex = (currentFrameIndex + 1) % swapchain->MAX_FRAME_IN_FLIGHT;
}

void RenderProcesser::beginSwapchainRenderPass(::vk::Framebuffer* buffer, ::vk::RenderPass* renderPass)
{
    assert(isFrameStart && "cat't call beginSwapchainRenderPass  is frame not in progress");
    swapchain->beginRenderPass(getCurrentCommadBuffer(), currentImageIndex, buffer, renderPass);
}

void RenderProcesser::endSwapchainRenderPass()
{
    getCurrentCommadBuffer().endRenderPass();
}

void RenderProcesser::allcoCmdBuffer()
{
    ::vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandPool(device_.getCommandPool())
        .setCommandBufferCount(swapchain->MAX_FRAME_IN_FLIGHT)
        .setLevel(::vk::CommandBufferLevel::ePrimary);
    
    commandBuffers_ = device_.logicalDevice().allocateCommandBuffers(allocInfo);
}


}
