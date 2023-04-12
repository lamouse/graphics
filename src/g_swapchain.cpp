#include "g_swapchain.hpp"
#include "g_defines.hpp"
#include <iostream>
namespace g
{

Swapchain::Swapchain(core::Device& device_, int width, int height):device_(device_)
{
    oldSwapchain = nullptr;
    init(width, height);
}

Swapchain::Swapchain(core::Device& device_, int width, int height, ::std::shared_ptr<Swapchain> oldSwapchain):device_(device_)
{
    this->oldSwapchain = oldSwapchain;
    init(width, height);
    oldSwapchain = nullptr;
}

void Swapchain::init(int width, int height)
{
    querySwapchainInfo(width, height);
    vk::SwapchainCreateInfoKHR createInfo;
    //裁切
    createInfo.setClipped(true)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setSurface(device_.getSurface())
            .setImageColorSpace(swapchainInfo.formatKHR.colorSpace)
            .setImageFormat(swapchainInfo.formatKHR.format)
            .setImageExtent(swapchainInfo.extent2D)
            .setMinImageCount(swapchainInfo.imageCount)
            .setPresentMode(swapchainInfo.presentMode);
    auto& queueIndices = device_.queueFamilyIndices;
    if(queueIndices.graphicsQueue.value() == queueIndices.presentQueue.value()){
        createInfo.setQueueFamilyIndexCount(queueIndices.graphicsQueue.value())
                    .setImageSharingMode(::vk::SharingMode::eExclusive);
    }else{
        ::std::array indices = {queueIndices.graphicsQueue.value(), queueIndices.presentQueue.value()};
        createInfo.setQueueFamilyIndices(indices)
                .setImageSharingMode(::vk::SharingMode::eConcurrent);
    }
    if(oldSwapchain != nullptr)
    {
        createInfo.setOldSwapchain(oldSwapchain->getSwapchain());
    }
    swapchain =device_.logicalDevice().createSwapchainKHR(createInfo);

    initRenderPass();
    createImageFrame();
    createsemphores();
    createFances();
}

void Swapchain::createImageFrame()
{
    createImageViews();
    createColorResources();
    createDepthResources();
    createFrameBuffers();
}


void Swapchain::querySwapchainInfo(int width, int height)
{
    auto& phyDevice = device_.getPhysicalDevice();
    auto& surface = device_.getSurface();
    auto  formats = phyDevice.getSurfaceFormatsKHR(surface);
    auto format = ::std::find_if(formats.begin(), formats.end(), [](auto format){
        return format.format == vk::Format::eR8G8B8A8Srgb && 
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    if(format != formats.end()){
        swapchainInfo.formatKHR = *format;
    } else {
         swapchainInfo.formatKHR = formats[0];
    }

    auto  cpabilities = phyDevice.getSurfaceCapabilitiesKHR(surface);
    swapchainInfo.imageCount = ::std::clamp<uint32_t>(2, cpabilities.minImageCount, cpabilities.maxImageCount);

    swapchainInfo.extent2D.width = ::std::clamp<uint32_t>(width, cpabilities.minImageExtent.width, cpabilities.maxImageExtent.width);
    swapchainInfo.extent2D.height = ::std::clamp<uint32_t>(height, cpabilities.minImageExtent.height, cpabilities.maxImageExtent.height);

    swapchainInfo.transForm = cpabilities.currentTransform;

    auto presents = phyDevice.getSurfacePresentModesKHR (surface);

    swapchainInfo.presentMode = chooseSwapPresentMode(presents);
}

::vk::PresentModeKHR Swapchain::chooseSwapPresentMode(const ::std::vector<::vk::PresentModeKHR>& availablePresentModes)
{


    //使用最新的图像，画面延迟更低，gpu不会闲置，功耗高
    for(const auto& availablePresentMode : availablePresentModes)
    {
        if(availablePresentMode == ::vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }

    // 不进行垂直同步，使用最高的cpu个gpu性能，能达到最高的fps，但可能造成画面撕裂
    // for(const auto& availablePresentMode : availablePresentModes)
    // {
    //     if(availablePresentMode == ::vk::PresentModeKHR::eImmediate)
    //     {
    //         return availablePresentMode;
    //     }
    // }

    //先进先出，排队
    return ::vk::PresentModeKHR::eFifo;
}
Swapchain::~Swapchain()
{

    auto& device = device_.logicalDevice();

    for(int i = 0; i < MAX_FRAME_IN_FLIGHT; i++){
        device.destroyFence(inFlightFences[i]);
        device.destroySemaphore(imageAvailableSemaphores[i]);
        device.destroySemaphore(renderFinshSemaphores[i]);
    }

    for(auto& frameBuffer : swapchainFrameBuffers){
        device.destroyFramebuffer(frameBuffer);
    }

    for(int i = 0; i < depthImages.size(); i++)
    {
        device.destroyImageView(depthImageViews[i]);
        device.freeMemory(depthImageMemorys[i]);
        device.destroyImage(depthImages[i]);
    }

    for(int i = 0; i < colorImages.size(); i++)
    {
        device.destroyImageView(colorImageViews[i]);
        device.freeMemory(colorImageMemorys[i]);
        device.destroyImage(colorImages[i]);
    }


    for(auto& view : imageViews){
        device.destroyImageView(view);
    }

    device.destroySwapchainKHR(swapchain);
    device.destroyRenderPass(renderPass_);

}

void Swapchain::getImages()
{
    images = device_.logicalDevice().getSwapchainImagesKHR(swapchain);
}

void Swapchain::createImageViews()
{
    getImages();
    imageViews.resize(images.size());
    for(int i = 0; i < images.size(); i++)
    {
        
        imageViews[i] =device_.createImageView(images[i], swapchainInfo.formatKHR.format, 
                ::vk::ImageAspectFlagBits::eColor, 1);
    }
}

void Swapchain::createFrameBuffers()
{
    swapchainFrameBuffers.resize(images.size());

    for(int i = 0; i < swapchainFrameBuffers.size(); i++)
    {
        ::vk::FramebufferCreateInfo createInfo;
        std::array<vk::ImageView, 3> attachments = {colorImageViews[i], depthImageViews[i], imageViews[i]};
        createInfo.setAttachments(attachments)
                .setWidth(swapchainInfo.extent2D.width)
                .setHeight(swapchainInfo.extent2D.height)
                .setRenderPass(renderPass_)
                .setLayers(1);
        swapchainFrameBuffers[i] = device_.logicalDevice().createFramebuffer(createInfo);
    }
}

::std::vector<::vk::Framebuffer> Swapchain::createFrameBuffer(int count, ::vk::RenderPass& renderPass)
{
    ::std::vector<::vk::Framebuffer> frameBuffers(count);

    for(int i = 0; i < frameBuffers.size(); i++)
    {
        ::vk::FramebufferCreateInfo createInfo;
        std::array<vk::ImageView, 1> attachments = { imageViews[i]};
        createInfo.setAttachments(attachments)
                .setWidth(swapchainInfo.extent2D.width)
                .setHeight(swapchainInfo.extent2D.height)
                .setRenderPass(renderPass)
                .setLayers(1);
        frameBuffers[i] = device_.logicalDevice().createFramebuffer(createInfo);
    }
    return frameBuffers;
}

void Swapchain::createColorResources()
{
    colorImages.resize(images.size());
    colorImageMemorys.resize(images.size());
    colorImageViews.resize(images.size());
    uint32_t mipLevels = 1;
    for(int i = 0; i < colorImages.size(); i++)
    {
        device_.createImage(swapchainInfo.extent2D.width, swapchainInfo.extent2D.height, mipLevels, swapchainInfo.formatKHR.format, 
                        device_.getMaxUsableSampleCount(), ::vk::ImageTiling::eOptimal, ::vk::ImageUsageFlagBits::eTransientAttachment | ::vk::ImageUsageFlagBits::eColorAttachment, 
                        ::vk::MemoryPropertyFlagBits::eDeviceLocal, colorImages[i], colorImageMemorys[i]);
        colorImageViews[i] = device_.createImageView(colorImages[i],  swapchainInfo.formatKHR.format, ::vk::ImageAspectFlagBits::eColor, mipLevels);
    }
}

void Swapchain::createDepthResources() {
    depthFormat = findDepthFormat();

    depthImages.resize(images.size());
    depthImageMemorys.resize(images.size());
    depthImageViews.resize(images.size());
    uint32_t mipLevels = 1;
    for (int i = 0; i < depthImages.size(); i++) {
        device_.createImage(swapchainInfo.extent2D.width, swapchainInfo.extent2D.height, mipLevels, depthFormat, 
                        device_.getMaxUsableSampleCount(), ::vk::ImageTiling::eOptimal, 
                        ::vk::ImageUsageFlagBits::eDepthStencilAttachment,
                        ::vk::MemoryPropertyFlagBits::eDeviceLocal, depthImages[i], depthImageMemorys[i]);
        depthImageViews[i] = device_.createImageView(depthImages[i],  depthFormat, ::vk::ImageAspectFlagBits::eDepth, mipLevels);
    }
}

void Swapchain::initRenderPass()
{
    ::vk::RenderPassCreateInfo createInfo;

    ::vk::AttachmentDescription colorDesc;
    colorDesc.setFormat(swapchainInfo.formatKHR.format)
                .setInitialLayout(::vk::ImageLayout::eUndefined)
                .setFinalLayout(::vk::ImageLayout::eColorAttachmentOptimal)
                .setLoadOp(::vk::AttachmentLoadOp::eClear)
                .setStoreOp(::vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
                .setSamples(device_.getMaxUsableSampleCount());

    ::vk::AttachmentDescription colorResolveDesc;
    colorResolveDesc.setFormat(swapchainInfo.formatKHR.format)
                .setInitialLayout(::vk::ImageLayout::eUndefined)
                .setFinalLayout(::vk::ImageLayout::ePresentSrcKHR)
                .setLoadOp(::vk::AttachmentLoadOp::eDontCare)
                .setStoreOp(::vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
                .setSamples(::vk::SampleCountFlagBits::e1);

    
    ::vk::AttachmentDescription depthAttachment;
    depthAttachment.setFormat(findDepthFormat())
                .setInitialLayout(::vk::ImageLayout::eUndefined)
                .setFinalLayout(::vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setFinalLayout(::vk::ImageLayout::ePresentSrcKHR)
                .setLoadOp(::vk::AttachmentLoadOp::eClear)
                .setStoreOp(::vk::AttachmentStoreOp::eDontCare)
                .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
                .setSamples(device_.getMaxUsableSampleCount());
    ::vk::AttachmentReference colorAttachmentReference;
    colorAttachmentReference.setLayout(::vk::ImageLayout::eColorAttachmentOptimal)
                        .setAttachment(0);
    vk::AttachmentReference depthAttachmentRef{};
    depthAttachmentRef.setAttachment(1)
                    .setLayout(::vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::AttachmentReference colorAttachmentResolveReference{};
    colorAttachmentResolveReference.setAttachment(2)
                    .setLayout(::vk::ImageLayout::eColorAttachmentOptimal);
    
    ::std::array<::vk::AttachmentDescription, 3> attachments = {colorDesc, depthAttachment, colorResolveDesc};
    createInfo.setAttachments(attachments);

     ::vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(::vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(colorAttachmentReference)
            .setPDepthStencilAttachment(&depthAttachmentRef)
            .setResolveAttachments(colorAttachmentResolveReference);
    createInfo.setSubpasses(subpass);

    ::vk::SubpassDependency subepassDependency;
    subepassDependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
                        .setDstSubpass(0)
                        .setDstAccessMask(::vk::AccessFlagBits::eColorAttachmentWrite | ::vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                        .setSrcAccessMask(::vk::AccessFlagBits::eNone)
                        .setDstStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput | ::vk::PipelineStageFlagBits::eEarlyFragmentTests)
                        .setSrcStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput | ::vk::PipelineStageFlagBits::eEarlyFragmentTests);
    createInfo.setDependencies(subepassDependency);
    renderPass_ = device_.logicalDevice().createRenderPass(createInfo);
}

 ::vk::Format Swapchain::findDepthFormat(){
    return device_.findSupportedFormat({::vk::Format::eD32Sfloat, ::vk::Format::eD32SfloatS8Uint, ::vk::Format::eD24UnormS8Uint},
            ::vk::ImageTiling::eOptimal,
            ::vk::FormatFeatureFlagBits::eDepthStencilAttachment);

 }

 void Swapchain::createFances()
{
    inFlightFences.resize(MAX_FRAME_IN_FLIGHT);
    for(int i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
    {
        ::vk::FenceCreateInfo info;
        info.setFlags(::vk::FenceCreateFlagBits::eSignaled);
        inFlightFences[i] = device_.logicalDevice().createFence(info);
    }
}

void Swapchain::createsemphores()
{
    imageAvailableSemaphores.resize(MAX_FRAME_IN_FLIGHT);
    renderFinshSemaphores.resize(MAX_FRAME_IN_FLIGHT);
    for(int i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
    {
        ::vk::SemaphoreCreateInfo semaphoreCreateInfo;
        imageAvailableSemaphores[i] = device_.logicalDevice().createSemaphore(semaphoreCreateInfo);

        ::vk::SemaphoreCreateInfo renderSemaphoreCreateInfo;
        renderFinshSemaphores[i] = device_.logicalDevice().createSemaphore(renderSemaphoreCreateInfo);
    }

}

::vk::ResultValue<uint32_t> Swapchain::acquireNextImage()
{
    auto device = device_.logicalDevice();

    auto waitResult = device.waitForFences(inFlightFences[currentFrame], VK_TRUE, ::std::numeric_limits<uint64_t>::max());
    if(waitResult != ::vk::Result::eSuccess)
    {
        throw ::std::runtime_error(" Swapchain::acquireNextImage wait fences");
    }

    auto result = device.acquireNextImageKHR(swapchain, ::std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame]);
    
    return result;
}

::vk::Result Swapchain::submitCommand(::vk::CommandBuffer& commandBuffer, uint32_t imageIndex)
{
    commandBuffer.end();
    ::vk::SubmitInfo submitInfo;
    ::vk::PipelineStageFlags stage = ::vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.setCommandBuffers(commandBuffer)
                .setWaitSemaphores(imageAvailableSemaphores[currentFrame])
                .setSignalSemaphores(renderFinshSemaphores[currentFrame])
                .setWaitDstStageMask(stage);
    device_.logicalDevice().resetFences(inFlightFences[currentFrame]);
    device_.getGraphicsQueue().submit(submitInfo, inFlightFences[currentFrame]);
    ::vk::PresentInfoKHR presentInfo;
    presentInfo.setImageIndices(imageIndex)
                .setSwapchains(swapchain)
                .setWaitSemaphores(renderFinshSemaphores[currentFrame]);

    auto result =device_.getPresentQueue().presentKHR(presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
    return result;
}

void Swapchain::beginRenderPass(::vk::CommandBuffer& commandBuffer, uint32_t imageIndex, ::vk::Framebuffer* buffer, ::vk::RenderPass* renderPass)
{
    auto extent = swapchainInfo.extent2D;
    ::vk::RenderPassBeginInfo renderPassBeginInfo;
    ::vk::Rect2D area;
    area.setOffset({0, 0})
        .setExtent(extent);
    
    //下标0 颜色附件，下标1深度附件
    ::std::array<::vk::ClearValue, 2> clearValues;
    clearValues[0].setColor(::vk::ClearColorValue(std::array<float, 4>({0.01f, 0.01f, 0.01f, 1.0f})));
    clearValues[1].setDepthStencil({1.0f, 0});

    auto drawBuffer = buffer == nullptr ? swapchainFrameBuffers[currentFrame] : buffer[imageIndex];
    auto drawRenderPass = renderPass == nullptr ? renderPass_ : *renderPass;
    renderPassBeginInfo.setRenderPass(drawRenderPass)
                        .setRenderArea(area)
                        .setFramebuffer(drawBuffer)
                        .setClearValues(clearValues);
    commandBuffer.beginRenderPass(renderPassBeginInfo, ::vk::SubpassContents::eInline);

    ::vk::Viewport viewPort;
    viewPort.setX(0.0f)
            .setY(0.0f)
            .setWidth(static_cast<float>(extent.width))
            .setHeight(static_cast<float>(extent.height))
            .setMinDepth(.0f)
            .setMaxDepth(1.f);
    ::vk::Rect2D scissor{{0, 0}, extent};    
    commandBuffer.setViewport(0, viewPort);
    commandBuffer.setScissor(0, scissor);
}

}
