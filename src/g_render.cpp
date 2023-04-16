#include "g_render.hpp"
#include "g_context.hpp"
#include <cassert>
#include <exception>
namespace g{
RenderProcesser::RenderProcesser(core::Device& device):device_(device)
{
    sampleCount_ = Context::Instance().imageQualityConfig.msaaSamples;
    createSwapchain();
    createRenderPass();
    swapchain->createFrameBuffers(renderPass_);
    allcoCmdBuffer();
}

RenderProcesser::~RenderProcesser()
{
    device_.logicalDevice().destroyRenderPass(renderPass_);
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
    ::vk::SampleCountFlagBits sampleCount = Context::Instance().imageQualityConfig.msaaSamples;
    if(swapchain == nullptr)
    {
        swapchain = ::std::make_unique<Swapchain>(device_, extent.width, extent.height, sampleCount);
    }else{
        ::std::shared_ptr<Swapchain> old = ::std::move(swapchain);
        swapchain = ::std::make_unique<Swapchain>(device_, extent.width, extent.height, sampleCount, old);
        swapchain->createFrameBuffers(renderPass_);
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
    // ::vk::CommandBufferInheritanceInfo inherritanceInfo;
    // begin.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    //     .setPInheritanceInfo(&inherritanceInfo);
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

void RenderProcesser::beginSwapchainRenderPass()
{
    assert(isFrameStart && "cat't call beginSwapchainRenderPass  is frame not in progress");
    swapchain->beginRenderPass(getCurrentCommadBuffer(), renderPass_, currentImageIndex);
}

void RenderProcesser::endSwapchainRenderPass()
{
    getCurrentCommadBuffer().endRenderPass();
    getCurrentCommadBuffer().end();
}

void RenderProcesser::allcoCmdBuffer()
{
    ::vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandPool(device_.getCommandPool())
        .setCommandBufferCount(swapchain->MAX_FRAME_IN_FLIGHT)
        .setLevel(::vk::CommandBufferLevel::ePrimary);
    
    commandBuffers_ = device_.logicalDevice().allocateCommandBuffers(allocInfo);
}

void RenderProcesser::createRenderPass()
{
    ::vk::AttachmentDescription colorAttachment;
    colorAttachment.setFormat(swapchain->getSwapchainColorFormat())
        .setSamples(sampleCount_)
        .setLoadOp(::vk::AttachmentLoadOp::eClear)
        .setStoreOp(::vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(::vk::ImageLayout::eUndefined)
        .setFinalLayout(::vk::ImageLayout::eColorAttachmentOptimal);

    ::vk::AttachmentDescription depthAttachment;
    depthAttachment.setFormat(swapchain->getSwapchainDepthFormat())
        .setSamples(sampleCount_)
        .setLoadOp(::vk::AttachmentLoadOp::eClear)
        .setStoreOp(::vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(::vk::ImageLayout::eUndefined)
        .setFinalLayout(::vk::ImageLayout::eDepthStencilAttachmentOptimal);

    ::vk::AttachmentDescription colorAttachmentResolve;
    colorAttachmentResolve.setFormat(swapchain->getSwapchainColorFormat())
        .setSamples(::vk::SampleCountFlagBits::e1)
        .setLoadOp(::vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(::vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(::vk::ImageLayout::eUndefined)
        .setFinalLayout(::vk::ImageLayout::ePresentSrcKHR);
    ::std::array<::vk::AttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};

    ::vk::AttachmentReference colorAttachmentRef(0, ::vk::ImageLayout::eColorAttachmentOptimal);
    ::vk::AttachmentReference depthAttachmentRef(1, ::vk::ImageLayout::eDepthStencilAttachmentOptimal);
    ::vk::AttachmentReference colorAttachmentResolveRef(2, ::vk::ImageLayout::eColorAttachmentOptimal);

    ::vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(::vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(colorAttachmentRef)
        .setPDepthStencilAttachment(&depthAttachmentRef)
        .setResolveAttachments(colorAttachmentResolveRef);

    ::vk::SubpassDependency subepassDependency;
    subepassDependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcAccessMask(::vk::AccessFlagBits::eNone)
        .setSrcStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput |
                         ::vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstAccessMask(::vk::AccessFlagBits::eColorAttachmentWrite |
                          ::vk::AccessFlagBits::eDepthStencilAttachmentWrite)
        .setDstStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput |
                         ::vk::PipelineStageFlagBits::eEarlyFragmentTests);

    ::vk::RenderPassCreateInfo createInfo;
    createInfo.setAttachments(attachments)
        .setSubpasses(subpass)
        .setDependencies(subepassDependency);
    renderPass_ = device_.logicalDevice().createRenderPass(createInfo);
}

}
