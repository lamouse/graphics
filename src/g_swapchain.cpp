#include "g_swapchain.hpp"

#include <spdlog/spdlog.h>

#include <gsl/gsl>

#include "g_defines.hpp"

namespace g {

Swapchain::Swapchain(core::Device& device_, int width, int height, ::vk::SampleCountFlagBits sampleCount,
                     ::std::shared_ptr<Swapchain> oldSwapchain)
    : device_(device_), sampleCount_(sampleCount) {
    init(width, height, oldSwapchain);
}

void Swapchain::init(int width, int height, ::std::shared_ptr<Swapchain>& oldSwapchain) {
    auto swapchainSupports = device_.querySwapchainSupport(device_.getPhysicalDevice());
    auto format = chooseSwapSurfaceFormat(swapchainSupports.formats);
    imageFormat = format.format;
    extent_ = chooseSwapExtent(swapchainSupports.capabilities, width, height);
    auto presentMode = chooseSwapPresentMode(swapchainSupports.presentModes);
    auto imageCount = ::std::clamp<uint32_t>(swapchainSupports.capabilities.minImageCount + 1,
                                             swapchainSupports.capabilities.minImageCount,
                                             swapchainSupports.capabilities.maxImageCount);

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.setClipped(true)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setSurface(device_.getSurface())
        .setImageColorSpace(format.colorSpace)
        .setImageFormat(imageFormat)
        .setImageExtent(extent_)
        .setMinImageCount(imageCount)
        .setPresentMode(presentMode)
        .setPreTransform(swapchainSupports.capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    const auto& queueIndices = device_.queueFamilyIndices;
    if (queueIndices.graphicsQueue.value() == queueIndices.presentQueue.value()) {
        createInfo.setImageSharingMode(::vk::SharingMode::eExclusive);
    } else {
        ::std::array indices = {queueIndices.graphicsQueue.value(), queueIndices.presentQueue.value()};
        createInfo.setQueueFamilyIndices(indices).setImageSharingMode(::vk::SharingMode::eConcurrent);
    }
    if (oldSwapchain != nullptr) {
        createInfo.setOldSwapchain(oldSwapchain->swapchain);
    }
    swapchain = device_.logicalDevice().createSwapchainKHR(createInfo);

    createImageFrame();
    createSemaphores();
    createFences();
}

void Swapchain::createImageFrame() {
    createImageViews();
    createColorResources();
    createDepthResources();
}

auto Swapchain::chooseSwapPresentMode(const ::std::vector<::vk::PresentModeKHR>& availablePresentModes)
    -> ::vk::PresentModeKHR {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == ::vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }

    return ::vk::PresentModeKHR::eFifo;
}
auto Swapchain::chooseSwapSurfaceFormat(const ::std::vector<::vk::SurfaceFormatKHR>& availableFormats)
    -> ::vk::SurfaceFormatKHR {
    const auto format = std::ranges::find_if(availableFormats, [](auto format) {
        return format.format == DEFAULT_COLOR_FORMAT && format.colorSpace == DEFAULT_COLOR_SPACE;
    });
    if (format != availableFormats.end()) {
        return *format;
    }
    return availableFormats[0];
}

auto Swapchain::chooseSwapExtent(const ::vk::SurfaceCapabilitiesKHR& capabilities, int width, int height)
    -> ::vk::Extent2D {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    ::vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

Swapchain::~Swapchain() {
    ::spdlog::debug(DETAIL_INFO("Swapchain"));
    const auto& device = device_.logicalDevice();
    for (::gsl::index i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        device.destroyFence(inFlightFences[i]);
        device.destroySemaphore(imageAvailableSemaphores[i]);
        device.destroySemaphore(renderFinishSemaphores[i]);
    }

    for (const auto& frameBuffer : frameBuffers) {
        device.destroyFramebuffer(frameBuffer);
    }

    for (::gsl::index i = 0; i < depthImages.size(); i++) {
        device.destroyImageView(depthImageViews[i]);
        device.freeMemory(depthImageMemories_[i]);
        device.destroyImage(depthImages[i]);
    }

    for (::gsl::index i = 0; i < colorImages.size(); i++) {
        device.destroyImageView(colorImageViews[i]);
        device.freeMemory(colorImageMemories[i]);
        device.destroyImage(colorImages[i]);
    }

    for (const auto& view : imageViews) {
        device.destroyImageView(view);
    }

    device.destroySwapchainKHR(swapchain);
}

void Swapchain::getImages() { images = device_.logicalDevice().getSwapchainImagesKHR(swapchain); }

void Swapchain::createImageViews() {
    getImages();
    imageViews.resize(images.size());
    for (::gsl::index i = 0; i < images.size(); i++) {
        imageViews[i] =
            device_.createImageView(images[i], getSwapchainColorFormat(), ::vk::ImageAspectFlagBits::eColor, 1);
    }
}

void Swapchain::createFrameBuffers(const ::vk::RenderPass& renderPass) {
    frameBuffers.resize(images.size());

    for (::gsl::index i = 0; i < frameBuffers.size(); i++) {
        ::vk::FramebufferCreateInfo createInfo;
        std::array<vk::ImageView, 3> attachments = {colorImageViews[i], depthImageViews[i], imageViews[i]};
        createInfo.setAttachments(attachments)
            .setWidth(extent_.width)
            .setHeight(extent_.height)
            .setRenderPass(renderPass)
            .setLayers(1);
        frameBuffers[i] = device_.logicalDevice().createFramebuffer(createInfo);
    }
}

void Swapchain::createColorResources() {
    colorImages.resize(images.size());
    colorImageMemories.resize(images.size());
    colorImageViews.resize(images.size());
    for (::gsl::index i = 0; i < colorImages.size(); i++) {
        device_.createImage(extent_.width, extent_.height, mipLevels, getSwapchainColorFormat(), sampleCount_,
                            ::vk::ImageTiling::eOptimal,
                            ::vk::ImageUsageFlagBits::eTransientAttachment | ::vk::ImageUsageFlagBits::eColorAttachment,
                            ::vk::MemoryPropertyFlagBits::eDeviceLocal, colorImages[i], colorImageMemories[i]);
        colorImageViews[i] = device_.createImageView(colorImages[i], getSwapchainColorFormat(),
                                                     ::vk::ImageAspectFlagBits::eColor, mipLevels);
    }
}

void Swapchain::createDepthResources() {
    depthFormat = findDepthFormat();

    depthImages.resize(images.size());
    depthImageMemories_.resize(images.size());
    depthImageViews.resize(images.size());
    for (::gsl::index i = 0; i < depthImages.size(); i++) {
        device_.createImage(extent_.width, extent_.height, mipLevels, depthFormat, sampleCount_,
                            ::vk::ImageTiling::eOptimal, ::vk::ImageUsageFlagBits::eDepthStencilAttachment,
                            ::vk::MemoryPropertyFlagBits::eDeviceLocal, depthImages[i], depthImageMemories_[i]);
        depthImageViews[i] =
            device_.createImageView(depthImages[i], depthFormat, ::vk::ImageAspectFlagBits::eDepth, mipLevels);
    }
}

auto Swapchain::findDepthFormat() const -> ::vk::Format {
    return device_.findSupportedFormat(
        {::vk::Format::eD32Sfloat, ::vk::Format::eD32SfloatS8Uint, ::vk::Format::eD24UnormS8Uint},
        ::vk::ImageTiling::eOptimal, ::vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void Swapchain::createFences() {
    inFlightFences.resize(MAX_FRAME_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        ::vk::FenceCreateInfo info;
        info.setFlags(::vk::FenceCreateFlagBits::eSignaled);
        inFlightFences[i] = device_.logicalDevice().createFence(info);
    }
}

void Swapchain::createSemaphores() {
    imageAvailableSemaphores.resize(MAX_FRAME_IN_FLIGHT);
    renderFinishSemaphores.resize(MAX_FRAME_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        ::vk::SemaphoreCreateInfo semaphoreCreateInfo;
        imageAvailableSemaphores[i] = device_.logicalDevice().createSemaphore(semaphoreCreateInfo);

        ::vk::SemaphoreCreateInfo renderSemaphoreCreateInfo;
        renderFinishSemaphores[i] = device_.logicalDevice().createSemaphore(renderSemaphoreCreateInfo);
    }
}

auto Swapchain::acquireNextImage() -> ::vk::ResultValue<uint32_t> {
    const auto& device = device_.logicalDevice();

    const auto waitResult =
        device.waitForFences(inFlightFences[currentFrame], VK_TRUE, ::std::numeric_limits<uint64_t>::max());
    if (waitResult != ::vk::Result::eSuccess) {
        throw ::std::runtime_error(" Swapchain::acquireNextImage wait fences");
    }
    device_.logicalDevice().resetFences(inFlightFences[currentFrame]);
    return device.acquireNextImageKHR(swapchain, ::std::numeric_limits<uint64_t>::max(),
                                      imageAvailableSemaphores[currentFrame]);
}

auto Swapchain::submitCommand(::vk::CommandBuffer& commandBuffer, uint32_t imageIndex) -> ::vk::Result {
    ::vk::SubmitInfo submitInfo;
    ::vk::PipelineStageFlags stage = ::vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.setCommandBuffers(commandBuffer)
        .setWaitSemaphores(imageAvailableSemaphores[currentFrame])
        .setSignalSemaphores(renderFinishSemaphores[currentFrame])
        .setWaitDstStageMask(stage);
    device_.getGraphicsQueue().submit(submitInfo, inFlightFences[currentFrame]);
    ::vk::PresentInfoKHR presentInfo;
    presentInfo.setImageIndices(imageIndex)
        .setSwapchains(swapchain)
        .setWaitSemaphores(renderFinishSemaphores[currentFrame]);

    const auto result = device_.getPresentQueue().presentKHR(presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
    return result;
}

void Swapchain::beginRenderPass(const ::vk::CommandBuffer& commandBuffer, const ::vk::RenderPass& renderPass,
                                uint32_t imageIndex) const {
    ::vk::RenderPassBeginInfo renderPassBeginInfo;
    ::vk::Rect2D area;
    area.setOffset({0, 0}).setExtent(extent_);

    ::std::array<::vk::ClearValue, 2> clearValues;
    clearValues[0].setColor(::vk::ClearColorValue(std::array<float, 4>({0.f, 0.f, 0.f, 1.0f})));
    clearValues[1].setDepthStencil({1.0f, 0});

    renderPassBeginInfo.setRenderPass(renderPass)
        .setRenderArea(area)
        .setFramebuffer(frameBuffers[imageIndex])
        .setClearValues(clearValues);
    commandBuffer.beginRenderPass(renderPassBeginInfo, ::vk::SubpassContents::eInline);

    ::vk::Viewport viewPort;
    viewPort.setX(0.0f)
        .setY(0.0f)
        .setWidth(static_cast<float>(extent_.width))
        .setHeight(static_cast<float>(extent_.height))
        .setMinDepth(.0f)
        .setMaxDepth(1.f);
    ::vk::Rect2D scissor{{0, 0}, extent_};
    commandBuffer.setViewport(0, viewPort);
    commandBuffer.setScissor(0, scissor);
}

}  // namespace g
