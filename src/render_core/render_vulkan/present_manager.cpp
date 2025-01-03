#include "present_manager.hpp"
#include "swapchain.hpp"
#include "vulkan_common/device.hpp"
#include "scheduler.hpp"
#include <spdlog/spdlog.h>
#include "common/settings.hpp"
#include "common/thread.hpp"
#include "vulkan_common/surface.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"
#include "common/microprofile.hpp"
#if defined min
#undef min
#endif
namespace render::vulkan {
MICROPROFILE_DEFINE(Vulkan_WaitPresent, "Vulkan", "Wait For Present", MP_RGB(128, 128, 128));
MICROPROFILE_DEFINE(Vulkan_CopyToSwapchain, "Vulkan", "Copy to swapchain", MP_RGB(192, 255, 192));
namespace {

auto canBlitToSwapchain(const vk::PhysicalDevice& physical_device, vk::Format format) -> bool {
    const vk::FormatProperties props = physical_device.getFormatProperties(format);
    return (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst) ==
           vk::FormatFeatureFlagBits::eBlitDst;
}
[[nodiscard]] auto makeImageSubresourceLayers() -> vk::ImageSubresourceLayers {
    return VkImageSubresourceLayers{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
}
[[nodiscard]] auto makeImageBlit(std::int32_t frame_width, std::int32_t frame_height,
                                 std::int32_t swapchain_width, std::int32_t swapchain_height)
    -> vk::ImageBlit {
    return VkImageBlit{
        .srcSubresource = makeImageSubresourceLayers(),
        .srcOffsets =
            {
                {
                    .x = 0,
                    .y = 0,
                    .z = 0,
                },
                {
                    .x = frame_width,
                    .y = frame_height,
                    .z = 1,
                },
            },
        .dstSubresource = makeImageSubresourceLayers(),
        .dstOffsets =
            {
                {
                    .x = 0,
                    .y = 0,
                    .z = 0,
                },
                {
                    .x = swapchain_width,
                    .y = swapchain_height,
                    .z = 1,
                },
            },
    };
}
[[nodiscard]] auto MakeImageCopy(uint32_t frame_width, uint32_t frame_height,
                                 uint32_t swapchain_width, uint32_t swapchain_height)
    -> vk::ImageCopy {
    return VkImageCopy{
        .srcSubresource = makeImageSubresourceLayers(),
        .srcOffset =
            {
                .x = 0,
                .y = 0,
                .z = 0,
            },
        .dstSubresource = makeImageSubresourceLayers(),
        .dstOffset =
            {
                .x = 0,
                .y = 0,
                .z = 0,
            },
        .extent =
            {
                .width = std::min(frame_width, swapchain_width),
                .height = std::min(frame_height, swapchain_height),
                .depth = 1,
            },
    };
}
}  // namespace

PresentManager::PresentManager(const vk::Instance& instance,
                               core::frontend::BaseWindow& render_window, const Device& device,
                               scheduler::Scheduler& scheduler_, Swapchain& swapchain,
                               SurfaceKHR& surface)
    : instance_{instance},
      render_window_{render_window},
      device_{device},
      scheduler_{scheduler_},
      swapchain_{swapchain},
      surface_{surface},
      blit_supported_{canBlitToSwapchain(device.getPhysical(), swapchain.getImageViewFormat())},
      use_present_thread_{common::settings::get<settings::RenderVulkan>().use_present_thread} {
    setImageCount();

    const auto& dld = device.getLogical();
    vk::CommandPoolCreateInfo cmdpool_info;
    cmdpool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
                          vk::CommandPoolCreateFlagBits::eTransient);
    cmdpool_ = dld.createCommandPool(cmdpool_info);
    vk::CommandBufferAllocateInfo cmdbuffer_info;
    cmdbuffer_info.setCommandBufferCount(static_cast<uint32_t>(image_count_))
        .setCommandPool(cmdpool_)
        .setLevel(vk::CommandBufferLevel::ePrimary);

    auto cmdbuffers = dld.allocateCommandBuffers(cmdbuffer_info);

    frames_.resize(image_count_);
    for (uint32_t i = 0; i < frames_.size(); i++) {
        Frame& frame = frames_[i];
        frame.cmdbuf = vk::CommandBuffer{cmdbuffers[i]};
        vk::SemaphoreCreateInfo semaphore_info;
        frame.render_ready = dld.createSemaphore(semaphore_info);
        vk::FenceCreateInfo fence_info;
        fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);
        frame.present_done = dld.createFence(fence_info);
        free_queue_.push(&frame);
    }

    if (use_present_thread_) {
        present_thread_ = std::jthread([this](std::stop_token token) { presentThread(token); });
    }
}

void PresentManager::setImageCount() {
    image_count_ = std::min<size_t>(swapchain_.getImageCount(), 7);
}

void PresentManager::presentThread(std::stop_token token) {
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
                surface_ =
                    render::vulkan::createSurface(instance_, render_window_.getWindowSystemInfo());
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
    MICROPROFILE_SCOPE(Vulkan_CopyToSwapchain);

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
    vk::CommandBufferBeginInfo begin_info;
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmdbuf.begin(begin_info);

    const vk::Image image{swapchain_.currentImage()};
    const vk::Extent2D extent = swapchain_.getExtent();
    const std::array pre_barriers{
        ::vk::ImageMemoryBarrier{
            vk::AccessFlagBits::eIndirectCommandRead,
            vk::AccessFlagBits::eTransferWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            image,
            vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor,
                0,
                1,
                0,
                VK_REMAINING_ARRAY_LAYERS,
            },
        },
        ::vk::ImageMemoryBarrier{

            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eTransferSrcOptimal,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            frame->image,
            vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor,
                0,
                1,
                0,
                VK_REMAINING_ARRAY_LAYERS,
            },

        },
    };
    const std::array post_barriers{
        ::vk::ImageMemoryBarrier{
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            image,
            vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor,
                0,
                1,
                0,
                VK_REMAINING_ARRAY_LAYERS,
            },
        },
        ::vk::ImageMemoryBarrier{

            vk::AccessFlagBits::eTransferRead,
            vk::AccessFlagBits::eMemoryWrite,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            frame->image,
            vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor,
                0,
                1,
                0,
                VK_REMAINING_ARRAY_LAYERS,
            },

        },
    };

    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                           vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, pre_barriers);

    if (blit_supported_) {
        cmdbuf.blitImage(frame->image, vk::ImageLayout::eTransferSrcOptimal, image,
                         vk::ImageLayout::eTransferDstOptimal,
                         makeImageBlit(frame->width, frame->height, extent.width, extent.height),
                         vk::Filter::eLinear);
    } else {
        cmdbuf.copyImage(frame->image, vk::ImageLayout::eTransferSrcOptimal, image,
                         vk::ImageLayout::eTransferDstOptimal,
                         MakeImageCopy(frame->width, frame->height, extent.width, extent.height));
    }

    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                           vk::PipelineStageFlagBits::eAllGraphics, {}, {}, {}, post_barriers);

    cmdbuf.end();

    const vk::Semaphore present_semaphore = swapchain_.currentPresentSemaphore();
    const vk::Semaphore render_semaphore = swapchain_.currentRenderSemaphore();
    const std::array wait_semaphores = {present_semaphore, frame->render_ready};

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
    std::scoped_lock submit_lock{scheduler_.submit_mutex_};
    try {
        device_.getGraphicsQueue().submit(submit_info, frame->present_done);

    } catch (const std::exception& e) {
        spdlog::error("Failed to submit image copy/blit to swapchain: {}", e.what());
    }

    // Present
    swapchain_.present(render_semaphore);
}

auto PresentManager::getRenderFrame() -> Frame* {
    MICROPROFILE_SCOPE(Vulkan_WaitPresent);

    // Wait for free presentation frames
    std::unique_lock lock{free_mutex_};
    free_cv_.wait(lock, [this] { return !free_queue_.empty(); });

    // Take the frame from the queue
    Frame* frame = free_queue_.front();
    free_queue_.pop();

    // Wait for the presentation to be finished so all frame resources are free
    auto result = device_.getLogical().waitForFences(frame->present_done, true, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        spdlog::error("Failed to wait for present done fence: {}", vk::to_string(result));
        throw utils::VulkanException(result);
    }
    device_.getLogical().resetFences(frame->present_done);

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
                                   vk::Format image_view_format, vk::RenderPass rd) {
    //   auto& dld = device_.getLogical();

    frame->width = width;
    frame->height = height;

    //    frame->image = memory_allocator.CreateImage({
    //        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    //        .pNext = nullptr,
    //        .flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
    //        .imageType = VK_IMAGE_TYPE_2D,
    //        .format = swapchain_.getImageFormat(),
    //        .extent =
    //            {
    //                .width = width,
    //                .height = height,
    //                .depth = 1,
    //            },
    //        .mipLevels = 1,
    //        .arrayLayers = 1,
    //        .samples = VK_SAMPLE_COUNT_1_BIT,
    //        .tiling = VK_IMAGE_TILING_OPTIMAL,
    //        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    //        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    //        .queueFamilyIndexCount = 0,
    //        .pQueueFamilyIndices = nullptr,
    //        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //        });
    //
    //    frame->image_view = dld.createImageView({
    //        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    //        .pNext = nullptr,
    //        .flags = 0,
    //        .image = *frame->image,
    //        .viewType = VK_IMAGE_VIEW_TYPE_2D,
    //        .format = image_view_format,
    //        .components =
    //            {
    //                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
    //                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
    //                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
    //                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    //            },
    //        .subresourceRange =
    //            {
    //                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    //                .baseMipLevel = 0,
    //                .levelCount = 1,
    //                .baseArrayLayer = 0,
    //                .layerCount = 1,
    //            },
    //        });
    //
    //    const VkImageView image_view{ *frame->image_view };
    //    frame->framebuffer = dld.createFramebuffer({
    //        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
    //        .pNext = nullptr,
    //        .flags = 0,
    //        .renderPass = rd,
    //        .attachmentCount = 1,
    //        .pAttachments = &image_view,
    //        .width = width,
    //        .height = height,
    //        .layers = 1,
    //        });
}

}  // namespace render::vulkan