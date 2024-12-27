#include "g_render.hpp"

#include <cassert>
#include <utility>

#include "core/device.hpp"

namespace g {
RenderProcessor::RenderProcessor(getScreenExtendFunc getScreenExtend) : getScreenExtend_(std::move(getScreenExtend)) {
    createSwapchain();
    createRenderPass();
    swapchain->createFrameBuffers(renderPass_);
    allocatorCmdBuffer();
}

RenderProcessor::~RenderProcessor() {
    core::Device device;
    device.logicalDevice().destroyRenderPass(renderPass_);
    device.logicalDevice().freeCommandBuffers(device.getCommandPool(), commandBuffers_);
}

void RenderProcessor::createSwapchain() {
    auto extent = getScreenExtend_();
    core::Device device;
    device.logicalDevice().waitIdle();
    ::vk::SampleCountFlagBits sampleCount = core::Device::getMaxMsaaSamples();
    if (swapchain == nullptr) {
        swapchain = ::std::make_unique<Swapchain>(extent.width, extent.height, sampleCount);
    } else {
        ::std::shared_ptr<Swapchain> old = ::std::move(swapchain);
        swapchain = ::std::make_unique<Swapchain>(extent.width, extent.height, sampleCount, old);
        swapchain->createFrameBuffers(renderPass_);
        if (!old->compareFormats(*swapchain)) {
            throw ::std::runtime_error("swapchain image(or depth) format has changed!");
        }
    }
}

auto RenderProcessor::beginFrame() -> bool {
    auto result = swapchain->acquireNextImage();

    if (result.result == ::vk::Result::eErrorOutOfDateKHR) {
        createSwapchain();
        return isFrameStart;
    }
    if (result.result != ::vk::Result::eSuccess && result.result != ::vk::Result::eSuboptimalKHR) {
        throw ::std::runtime_error("fail acquire swapchain image");
    }

    isFrameStart = true;
    currentImageIndex = result.value;
    getCurrentCommandBuffer().reset();
    ::vk::CommandBufferBeginInfo begin;
    // ::vk::CommandBufferInheritanceInfo inherritanceInfo;
    // begin.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    //     .setPInheritanceInfo(&inherritanceInfo);
    getCurrentCommandBuffer().begin(begin);
    return isFrameStart;
}

void RenderProcessor::endFrame() {
    assert(isFrameStart && "can't call begin swapchain render pass is frame not in progress");
    try {
        const auto result = swapchain->submitCommand(getCurrentCommandBuffer(), currentImageIndex);

        if (result == ::vk::Result::eErrorOutOfDateKHR || result == ::vk::Result::eSuboptimalKHR) {
            createSwapchain();
        } else if (result != ::vk::Result::eSuccess) {
            throw ::std::runtime_error("filed to present swap chain image");
        }
    } catch (const ::vk::OutOfDateKHRError&) {
        createSwapchain();
    }

    isFrameStart = false;
    currentFrameIndex = (currentFrameIndex + 1) % swapchain->MAX_FRAME_IN_FLIGHT;
}

void RenderProcessor::beginSwapchainRenderPass() {
    assert(isFrameStart && "can't call beginSwapchainRenderPass  is frame not in progress");
    swapchain->beginRenderPass(getCurrentCommandBuffer(), renderPass_, currentImageIndex);
}

void RenderProcessor::endSwapchainRenderPass() {
    getCurrentCommandBuffer().endRenderPass();
    getCurrentCommandBuffer().end();
}

void RenderProcessor::allocatorCmdBuffer() {
    core::Device device;
    ::vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandPool(device.getCommandPool())
        .setCommandBufferCount(swapchain->MAX_FRAME_IN_FLIGHT)
        .setLevel(::vk::CommandBufferLevel::ePrimary);

    commandBuffers_ = device.logicalDevice().allocateCommandBuffers(allocInfo);
}

void RenderProcessor::createRenderPass() {
    core::Device device;
    ::vk::AttachmentDescription colorAttachment;
    colorAttachment.setFormat(swapchain->getSwapchainColorFormat())
        .setSamples(core::Device::getMaxMsaaSamples())
        .setLoadOp(::vk::AttachmentLoadOp::eClear)
        .setStoreOp(::vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(::vk::ImageLayout::eUndefined)
        .setFinalLayout(::vk::ImageLayout::eColorAttachmentOptimal);

    ::vk::AttachmentDescription depthAttachment;
    depthAttachment.setFormat(swapchain->getSwapchainDepthFormat())
        .setSamples(core::Device::getMaxMsaaSamples())
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
    ::std::array<::vk::AttachmentDescription, 3> attachments = {colorAttachment, depthAttachment,
                                                                colorAttachmentResolve};

    ::vk::AttachmentReference colorAttachmentRef(0, ::vk::ImageLayout::eColorAttachmentOptimal);
    ::vk::AttachmentReference depthAttachmentRef(1, ::vk::ImageLayout::eDepthStencilAttachmentOptimal);
    ::vk::AttachmentReference colorAttachmentResolveRef(2, ::vk::ImageLayout::eColorAttachmentOptimal);

    ::vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(::vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(colorAttachmentRef)
        .setPDepthStencilAttachment(&depthAttachmentRef)
        .setResolveAttachments(colorAttachmentResolveRef);

    ::vk::SubpassDependency subpassDependency;
    subpassDependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcAccessMask(::vk::AccessFlagBits::eNone)
        .setSrcStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput |
                         ::vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstAccessMask(::vk::AccessFlagBits::eColorAttachmentWrite |
                          ::vk::AccessFlagBits::eDepthStencilAttachmentWrite)
        .setDstStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput |
                         ::vk::PipelineStageFlagBits::eEarlyFragmentTests);

    ::vk::RenderPassCreateInfo createInfo;
    createInfo.setAttachments(attachments).setSubpasses(subpass).setDependencies(subpassDependency);
    renderPass_ = device.logicalDevice().createRenderPass(createInfo);
}

}  // namespace g
