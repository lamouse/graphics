module;
#include <spdlog/spdlog.h>
#include "common/settings.hpp"
#include "common/thread.hpp"
#include <vulkan/vulkan.hpp>
#include <tracy/Tracy.hpp>
#if defined min
#undef min
#endif
module render.vulkan.present_manager;
import render.vulkan.common;
import render.vulkan.utils;
import render.vulkan.scheduler;
import render.vulkan.swapchain;
import core;

namespace render::vulkan {
namespace {

auto canBlitToSwapchain(const vk::PhysicalDevice& physical_device, vk::Format format) -> bool {
    const vk::FormatProperties props = physical_device.getFormatProperties(format);
    return (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst) ==
           vk::FormatFeatureFlagBits::eBlitDst;
}
[[nodiscard]] auto makeImageSubresourceLayers() -> vk::ImageSubresourceLayers {
    return vk::ImageSubresourceLayers()
        .setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setLayerCount(1);
}
[[nodiscard]] auto makeImageBlit(std::int32_t frame_width, std::int32_t frame_height,
                                 std::int32_t swapchain_width, std::int32_t swapchain_height)
    -> vk::ImageBlit {
    return vk::ImageBlit()
        .setSrcSubresource(makeImageSubresourceLayers())
        .setDstSubresource(makeImageSubresourceLayers())
        .setSrcOffsets(std::array<vk::Offset3D, 2>{
            vk::Offset3D().setX(0).setY(0).setZ(0),
            vk::Offset3D().setX(frame_width).setY(frame_height).setZ(1)})
        .setDstOffsets(std::array<vk::Offset3D, 2>{
            vk::Offset3D().setX(0).setY(0).setZ(0),
            vk::Offset3D().setX(swapchain_width).setY(swapchain_height).setZ(1)});
}
[[nodiscard]] auto MakeImageCopy(uint32_t frame_width, uint32_t frame_height,
                                 uint32_t swapchain_width, uint32_t swapchain_height)
    -> vk::ImageCopy {
    return vk::ImageCopy()
        .setSrcSubresource(makeImageSubresourceLayers())
        .setDstSubresource(makeImageSubresourceLayers())
        .setExtent(vk::Extent3D()
                       .setWidth(std::min(frame_width, swapchain_width))
                       .setHeight(std::min(frame_height, swapchain_height))
                       .setDepth(1));
}
}  // namespace

PresentManager::PresentManager(const Instance& instance,
                               core::frontend::BaseWindow& render_window, const Device& device,
                               MemoryAllocator& memory_allocator, scheduler::Scheduler& scheduler_,
                               Swapchain& swapchain, SurfaceKHR& surface)
    : instance_(instance),
      render_window_{render_window},
      device_{device},
      memory_allocator_(memory_allocator),
      scheduler_{scheduler_},
      swapchain_{swapchain},
      surface_{surface},
      blit_supported_{canBlitToSwapchain(device.getPhysical(), swapchain.getImageViewFormat())},
      use_present_thread_{settings::values.use_present_thread.GetValue()} {
    setImageCount();
    cmdpool_ = device.logical().createCommandPool(
        vk::CommandPoolCreateInfo().setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
                                             vk::CommandPoolCreateFlagBits::eTransient));

    auto cmdbuffers = cmdpool_.Allocate(image_count_);

    frames_.resize(image_count_);
    for (uint32_t i = 0; i < frames_.size(); i++) {
        Frame& frame = frames_[i];
        frame.cmdbuf = vk::CommandBuffer{cmdbuffers[i]};
        frame.render_ready = device_.logical().createSemaphore();

        frame.present_done = device_.logical().createFence(
            vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
        free_queue_.push(&frame);
    }

    if (use_present_thread_) {
        present_thread_ =
            std::jthread([this](std::stop_token token) { presentThread(token); });  // NOLINT
    }
}

void PresentManager::setImageCount() {
    image_count_ = std::min<size_t>(swapchain_.getImageCount(), 7);
}

void PresentManager::presentThread(std::stop_token token) {  // NOLINT
    common::thread::SetCurrentThreadName("VulkanPresent");
    while (!token.stop_requested()) {
        std::unique_lock lock{queue_mutex_};

        // Wait for presentation frames
        common::thread::condvarWait(frame_cv_, lock, token,
                                    [this] { return !present_queue_.empty(); });
        if (token.stop_requested()) {
            return;
        }

        // Take the frame and notify anyone waiting
        Frame* frame = present_queue_.front();
        present_queue_.pop();
        frame_cv_.notify_one();

        // By exchanging the lock ownership we take the swapchain lock
        // before the queue lock goes out of scope. This way the swapchain
        // lock in WaitPresent is guaranteed to occur after here.
        std::exchange(lock, std::unique_lock{swapchain_mutex_});

        copyToSwapchain(frame);

        // Free the frame for reuse
        std::scoped_lock fl{free_mutex_};
        free_queue_.push(frame);
        free_cv_.notify_one();
    }
}

void PresentManager::recreateSwapchain(Frame* frame) {
    swapchain_.create(*surface_, frame->width, frame->height);
    setImageCount();
}
void PresentManager::copyToSwapchain(Frame* frame) {
    bool requires_recreation = false;

    while (true) {
        try {
            // Recreate surface and swapchain if needed.
            if (requires_recreation) {
                 surface_ = createSurface(instance_, render_window_.getWindowSystemInfo());
                recreateSwapchain(frame);
            }

            // Draw to swapchain.
            return copyToSwapchainImpl(frame);
        } catch (const utils::VulkanException& except) {
            if (except.getResult() != vk::Result::eErrorSurfaceLostKHR) {
                throw;
            }

            requires_recreation = true;
        }
    }
}

void PresentManager::copyToSwapchainImpl(Frame* frame) {
    ZoneScopedN("PresentManager::copyToSwapchainImpl()");
    // If the size of the incoming frames has changed, recreate the swapchain
    // to account for that.
    const bool is_suboptimal = swapchain_.needsRecreation();
    const bool size_changed =
        swapchain_.getWidth() != frame->width || swapchain_.getHeight() != frame->height;
    if (is_suboptimal || size_changed) {
        recreateSwapchain(frame);
    }

    while (swapchain_.acquireNextImage()) {
        recreateSwapchain(frame);
    }

    const vk::CommandBuffer cmdbuf{frame->cmdbuf};
    cmdbuf.begin(
        vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    const vk::Image image{swapchain_.currentImage()};
    const vk::Extent2D extent = swapchain_.getExtent();
    const std::array pre_barriers{
        ::vk::ImageMemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eIndirectCommandRead)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(image)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(VK_REMAINING_ARRAY_LAYERS)),

        ::vk::ImageMemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(*frame->image)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(VK_REMAINING_ARRAY_LAYERS)),
    };
    const std::array post_barriers{
        ::vk::ImageMemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(image)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(VK_REMAINING_ARRAY_LAYERS)),
        ::vk::ImageMemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstAccessMask(vk::AccessFlagBits::eMemoryWrite)
            .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(*frame->image)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(VK_REMAINING_ARRAY_LAYERS)),
    };

    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                           vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, pre_barriers);

    if (blit_supported_) {
        cmdbuf.blitImage(*frame->image, vk::ImageLayout::eTransferSrcOptimal, image,
                         vk::ImageLayout::eTransferDstOptimal,
                         makeImageBlit(frame->width, frame->height, extent.width, extent.height),
                         vk::Filter::eLinear);
    } else {
        cmdbuf.copyImage(*frame->image, vk::ImageLayout::eTransferSrcOptimal, image,
                         vk::ImageLayout::eTransferDstOptimal,
                         MakeImageCopy(frame->width, frame->height, extent.width, extent.height));
    }

    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                           vk::PipelineStageFlagBits::eAllGraphics, {}, {}, {}, post_barriers);

    cmdbuf.end();

    const vk::Semaphore present_semaphore = swapchain_.currentPresentSemaphore();
    const vk::Semaphore render_semaphore = swapchain_.currentRenderSemaphore();
    const std::array wait_semaphores = {present_semaphore, *frame->render_ready};

    static constexpr std::array<vk::PipelineStageFlags, 2> wait_stage_masks{
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eAllCommands,
    };

    vk::SubmitInfo submit_info;
    submit_info.setWaitSemaphores(wait_semaphores)
        .setWaitDstStageMask(wait_stage_masks)
        .setCommandBuffers(cmdbuf)
        .setSignalSemaphores(render_semaphore);

    // Submit the image copy/blit to the swapchain
    {
        std::scoped_lock submit_lock{scheduler_.submit_mutex_};
        try {
            device_.getGraphicsQueue().submit(submit_info, *frame->present_done);

        } catch (const std::exception& e) {
            spdlog::error("Failed to submit image copy/blit to swapchain: {}", e.what());
        }
    }

    // Present
    swapchain_.present(render_semaphore);
}

auto PresentManager::getRenderFrame() -> Frame* {
    ZoneScopedN("PresentManager::getRenderFrame()");
    // Wait for free presentation frames
    std::unique_lock lock{free_mutex_};
    free_cv_.wait(lock, [this] { return !free_queue_.empty(); });

    // Take the frame from the queue
    Frame* frame = free_queue_.front();
    free_queue_.pop();

    // Wait for the presentation to be finished so all frame resources are free
    auto result = frame->present_done.Wait();
    if (result != vk::Result::eSuccess) {
        spdlog::error("Failed to wait for present done fence: {}", vk::to_string(result));
        throw utils::VulkanException(result);
    }
    frame->present_done.Reset();

    return frame;
}

void PresentManager::present(Frame* frame) {
    if (!use_present_thread_) {
        scheduler_.waitWorker();
        copyToSwapchain(frame);
        free_queue_.push(frame);
        return;
    }

    scheduler_.record([this, frame](vk::CommandBuffer) {
        std::unique_lock lock{queue_mutex_};
        present_queue_.push(frame);
        frame_cv_.notify_one();
    });
}

void PresentManager::waitPresent() {
    if (!use_present_thread_) {
        return;
    }

    // Wait for the present queue to be empty
    {
        std::unique_lock queue_lock{queue_mutex_};
        frame_cv_.wait(queue_lock, [this] { return present_queue_.empty(); });
    }

    // The above condition will be satisfied when the last frame is taken from the queue.
    // To ensure that frame has been presented as well take hold of the swapchain
    // mutex.
    std::scoped_lock swapchain_lock{swapchain_mutex_};
}

void PresentManager::recreateFrame(Frame* frame, u32 width, u32 height,
                                   vk::Format image_view_format) {
    frame->width = width;
    frame->height = height;

    frame->image = memory_allocator_.createImage({
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = static_cast<VkFormat>(swapchain_.getImageFormat()),
        .extent =
            {
                .width = width,
                .height = height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    });
    scheduler_.record([frame_image = *frame->image](vk::CommandBuffer cmdbuf) {
        present::utils::TransitionImageLayout(cmdbuf, frame_image, vk::ImageLayout::eGeneral,
                                              vk::ImageLayout::eUndefined);
    });
    frame->image_view = device_.logical().CreateImageView(
        vk::ImageViewCreateInfo()
            .setImage(*frame->image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(image_view_format)
            .setComponents(vk::ComponentMapping()
                               .setR(vk::ComponentSwizzle::eIdentity)
                               .setG(vk::ComponentSwizzle::eIdentity)
                               .setB(vk::ComponentSwizzle::eIdentity)
                               .setA(vk::ComponentSwizzle::eIdentity))
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)));
}

}  // namespace render::vulkan