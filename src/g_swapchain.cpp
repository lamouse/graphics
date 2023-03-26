#include "g_swapchain.hpp"
#include "g_device.hpp"
#include "g_pipeline.hpp"
#include <iostream>
namespace g
{

Swapchain::Swapchain( int width, int height)
{
    oldSwapchain = nullptr;
    init(width, height);
}

Swapchain::Swapchain(int width, int height, ::std::shared_ptr<Swapchain> oldSwapchain)
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
            .setSurface(Device::getInstance().getSurface())
            .setImageColorSpace(swapchainInfo.formatKHR.colorSpace)
            .setImageFormat(swapchainInfo.formatKHR.format)
            .setImageExtent(swapchainInfo.extent2D)
            .setMinImageCount(swapchainInfo.imageCount)
            .setPresentMode(swapchainInfo.presentMode);
    auto& queueIndices = Device::getInstance().queueFamilyIndices;
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
    swapchain = Device::getInstance().getVKDevice().createSwapchainKHR(createInfo);

    initRenderPass();
    createImageFrame();
    createsemphores();
    createFances();
}

void Swapchain::createImageFrame()
{
    createImageViews();
    createDepthResources();
    createFrameBuffers();
}


void Swapchain::querySwapchainInfo(int width, int height)
{
    auto& phyDevice = Device::getInstance().getPhysicalDevice();
    auto& surface = Device::getInstance().getSurface();
    auto  formats = phyDevice.getSurfaceFormatsKHR(surface);
    auto format = ::std::find_if(formats.begin(), formats.end(), [](auto format){
        return format.format == vk::Format::eB8G8R8A8Srgb && 
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

    for(auto & fence : inFlightFences){
        Device::getInstance().getVKDevice().destroyFence(fence);
    }

    for(auto & semaphore : imageAvailableSemaphores){
        Device::getInstance().getVKDevice().destroySemaphore(semaphore);
    }

    for(auto& frameBuffer : frameBuffers){
        Device::getInstance().getVKDevice().destroyFramebuffer(frameBuffer);
    }

    for(auto& depthImageView : depthImageViews){
        Device::getInstance().getVKDevice().destroyImageView(depthImageView);
    }
    for(auto& depthImageMemory : depthImageMemorys){
        Device::getInstance().getVKDevice().freeMemory(depthImageMemory);
    }
    for(auto& depthImage : depthImages){
        Device::getInstance().getVKDevice().destroyImage(depthImage);
    }
    for(auto& view : imageViews){
        Device::getInstance().getVKDevice().destroyImageView(view);
    }
    
    Device::getInstance().getVKDevice().destroySwapchainKHR(swapchain);
    Device::getInstance().getVKDevice().destroyRenderPass(renderPass);

}

void Swapchain::getImages()
{
    images = Device::getInstance().getVKDevice().getSwapchainImagesKHR(swapchain);
}

void Swapchain::createImageViews()
{
    getImages();
    imageViews.resize(images.size());
    for(int i = 0; i < images.size(); i++)
    {
        ::vk::ComponentMapping mapping;
        ::vk::ImageViewCreateInfo createImageViewInfo;
        ::vk::ImageSubresourceRange range;
        range.setBaseMipLevel(0)
            .setLevelCount(VK_REMAINING_MIP_LEVELS)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setAspectMask(vk::ImageAspectFlagBits::eColor);
        createImageViewInfo.setImage(images[i])
                            .setViewType(vk::ImageViewType::e2D)
                            .setComponents(mapping)
                            .setSubresourceRange(range)
                            .setFormat(swapchainInfo.formatKHR.format);
        imageViews[i] = Device::getInstance().getVKDevice().createImageView(createImageViewInfo);
    }
}

void Swapchain::createFrameBuffers()
{
    frameBuffers.resize(images.size());

    for(int i = 0; i < frameBuffers.size(); i++)
    {
        ::vk::FramebufferCreateInfo createInfo;
        std::array<vk::ImageView, 2> attachments = {imageViews[i], depthImageViews[i]};
        createInfo.setAttachments(attachments)
                .setWidth(swapchainInfo.extent2D.width)
                .setHeight(swapchainInfo.extent2D.height)
                .setRenderPass(renderPass)
                .setLayers(1);
        frameBuffers[i] = Device::getInstance().getVKDevice().createFramebuffer(createInfo);
    }
}


void Swapchain::createDepthResources() {
  depthFormat = findDepthFormat();

  depthImages.resize(images.size());
  depthImageMemorys.resize(images.size());
  depthImageViews.resize(images.size());

  for (int i = 0; i < depthImages.size(); i++) {
    ::vk::ImageCreateInfo imageInfo{};
    imageInfo.setImageType(::vk::ImageType::e2D);
    ::vk::Extent3D extent{swapchainInfo.extent2D.width, swapchainInfo.extent2D.height, 1};
    imageInfo.setExtent(extent)
             .setMipLevels(1)
             .setArrayLayers(1)
             .setFormat(depthFormat)
             .setTiling(::vk::ImageTiling::eOptimal)
             .setInitialLayout(::vk::ImageLayout::eUndefined)
             .setUsage(::vk::ImageUsageFlagBits::eDepthStencilAttachment)
             .setSamples(::vk::SampleCountFlagBits::e1)
             .setSharingMode(::vk::SharingMode::eExclusive);

    depthImages[i] =  Device::getInstance().getVKDevice().createImage(imageInfo);
    ::vk::MemoryRequirements requirement = Device::getInstance().getVKDevice().getImageMemoryRequirements(depthImages[i]);
    ::vk::MemoryAllocateInfo allocInfo;
    allocInfo.setAllocationSize(requirement.size)
            .setMemoryTypeIndex(Device::getInstance().findMemoryType(requirement.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

    depthImageMemorys[i] = Device::getInstance().getVKDevice().allocateMemory(allocInfo);
    Device::getInstance().getVKDevice().bindImageMemory(depthImages[i], depthImageMemorys[i], 0);
    ::vk::ImageSubresourceRange imageSubresource;
    imageSubresource.setAspectMask(::vk::ImageAspectFlagBits::eDepth)
                    .setBaseMipLevel(0)
                    .setLevelCount(1)
                    .setLayerCount(1)
                    .setBaseArrayLayer(0);

    ::vk::ComponentMapping mapping;
    ::vk::ImageViewCreateInfo viewInfo;
    viewInfo.setImage(depthImages[i])
            .setViewType(::vk::ImageViewType::e2D)
            .setFormat(depthFormat)
            .setSubresourceRange(imageSubresource)
            .setComponents(mapping);

    depthImageViews[i] =  Device::getInstance().getVKDevice().createImageView(viewInfo);
  }
}

void Swapchain::initRenderPass()
{
    ::vk::RenderPassCreateInfo createInfo;
    ::vk::AttachmentDescription colorDesc;
    colorDesc.setFormat(swapchainInfo.formatKHR.format)
                .setInitialLayout(::vk::ImageLayout::eUndefined)
                .setFinalLayout(::vk::ImageLayout::ePresentSrcKHR)
                .setLoadOp(::vk::AttachmentLoadOp::eClear)
                .setStoreOp(::vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
                .setSamples(::vk::SampleCountFlagBits::e1);
    ::vk::AttachmentReference colorAttachmentReference;
    colorAttachmentReference.setLayout(::vk::ImageLayout::eColorAttachmentOptimal)
                        .setAttachment(0);
    
    ::vk::AttachmentDescription depthAttachment;
    depthAttachment.setFormat(findDepthFormat())
                .setInitialLayout(::vk::ImageLayout::eUndefined)
                .setFinalLayout(::vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setFinalLayout(::vk::ImageLayout::ePresentSrcKHR)
                .setLoadOp(::vk::AttachmentLoadOp::eClear)
                .setStoreOp(::vk::AttachmentStoreOp::eDontCare)
                .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
                .setSamples(::vk::SampleCountFlagBits::e1);
    vk::AttachmentReference depthAttachmentRef{};
    depthAttachmentRef.setAttachment(1)
                    .setLayout(::vk::ImageLayout::eDepthStencilAttachmentOptimal);
    
    ::std::array<::vk::AttachmentDescription, 2> attachments = {colorDesc, depthAttachment};
    createInfo.setAttachments(attachments);

     ::vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(::vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(colorAttachmentReference)
            .setPDepthStencilAttachment(&depthAttachmentRef);
    createInfo.setSubpasses(subpass);

    ::vk::SubpassDependency subepassDependency;
    subepassDependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
                        .setDstSubpass(0)
                        .setDstAccessMask(::vk::AccessFlagBits::eColorAttachmentWrite | ::vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                        .setSrcAccessMask(::vk::AccessFlagBits::eNone)
                        .setDstStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput | ::vk::PipelineStageFlagBits::eEarlyFragmentTests)
                        .setSrcStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput | ::vk::PipelineStageFlagBits::eEarlyFragmentTests);
    createInfo.setDependencies(subepassDependency);
    renderPass = Device::getInstance().getVKDevice().createRenderPass(createInfo);
}

 ::vk::Format Swapchain::findDepthFormat(){
    return Device::getInstance().findSupportedFormat({::vk::Format::eD32Sfloat, ::vk::Format::eD32SfloatS8Uint, ::vk::Format::eD24UnormS8Uint},
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
        inFlightFences[i] = Device::getInstance().getVKDevice().createFence(info);
    }
}

void Swapchain::createsemphores()
{
    imageAvailableSemaphores.resize(MAX_FRAME_IN_FLIGHT);
    for(int i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
    {
        ::vk::SemaphoreCreateInfo semaphoreCreateInfo;
        imageAvailableSemaphores[i] = Device::getInstance().getVKDevice().createSemaphore(semaphoreCreateInfo);

    }

}

::vk::ResultValue<uint32_t> Swapchain::acquireNextImage()
{
    auto device = Device::getInstance().getVKDevice();

    auto waitResult = device.waitForFences(inFlightFences[currentFrame], VK_TRUE, ::std::numeric_limits<uint64_t>::max());
    if(waitResult != ::vk::Result::eSuccess)
    {
        throw ::std::runtime_error(" Swapchain::acquireNextImage wait fences");
    }

    auto result = device.acquireNextImageKHR(swapchain, ::std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame]);
    
    return result;
}

::vk::Result Swapchain::submitCommand(Command& command, uint32_t imageIndex, int bufferIndex)
{
   auto result =  command.end(bufferIndex, imageIndex, swapchain, imageAvailableSemaphores[currentFrame], inFlightFences[currentFrame]);
    currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
    return result;
}

void Swapchain::beginRenderPass(Command& command, uint32_t imageIndex)
{
    command.beginRenderPass(imageIndex, renderPass, swapchainInfo.extent2D, frameBuffers[currentFrame]);
}

}
