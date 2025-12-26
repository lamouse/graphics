module;
#include <spdlog/spdlog.h>
#include "common/polyfill_thread.hpp"
#include <boost/container/small_vector.hpp>
#include <tracy/Tracy.hpp>
#include <vulkan/vulkan.hpp>
#include "common/settings.hpp"
#ifdef MemoryBarrier
#undef MemoryBarrier
#endif
module render.vulkan;
import render.vulkan.common;
import :scheduler;

namespace render::vulkan::scheduler {

void Scheduler::CommandChunk::executeAll(vk::CommandBuffer cmdbuf,
                                         vk::CommandBuffer upload_cmdbuf) {
    auto* command = first;
    while (command != nullptr) {
        auto* next = command->getNext();
        command->execute(cmdbuf, upload_cmdbuf);
        command->~Command();
        command = next;
    }
    submit = false;
    command_offset = 0;
    first = nullptr;
    last = nullptr;
}

Scheduler::Scheduler(const Device& device)
    : device_{device},
      master_semaphore_{std::make_unique<semaphore::MasterSemaphore>(device)},
      command_pool_{std::make_unique<resource::CommandPool>(master_semaphore_.get(), device)},
      use_dynamic_rendering(settings::values.use_dynamic_rendering.GetValue()) {
    acquireNewChunk();
    allocateWorkerCommandBuffer();
    worker_thread_ = std::jthread([this](std::stop_token token) { workerThread(token); });
}

void Scheduler::workerThread(std::stop_token stop_token) {
    const auto TryPopQueue{[this](auto& work) -> bool {
        if (work_queue_.empty()) {
            return false;
        }

        work = std::move(work_queue_.front());
        work_queue_.pop();
        event_cv_.notify_all();
        return true;
    }};

    while (!stop_token.stop_requested()) {
        std::unique_ptr<CommandChunk> work;

        {
            std::unique_lock lk{queue_mutex_};

            // Wait for work.
            common::thread::condvarWait(event_cv_, lk, stop_token,
                                        [&] { return TryPopQueue(work); });

            // If we've been asked to stop, we're done.
            if (stop_token.stop_requested()) {
                return;
            }

            // Exchange lock ownership so that we take the execution lock before
            // the queue lock goes out of scope. This allows us to force execution
            // to complete in the next step.
            std::exchange(lk, std::unique_lock{execution_mutex_});

            // Perform the work, tracking whether the chunk was a submission
            // before executing.
            const bool has_submit = work->hasSubmit();
            work->executeAll(current_cmd_buf_, current_upload_cmd_buf_);

            // If the chunk was a submission, reallocate the command buffer.
            if (has_submit) {
                allocateWorkerCommandBuffer();
            }
        }

        {
            std::scoped_lock rl{reserve_mutex_};

            // Recycle the chunk back to the reserve.
            chunk_reserve_.emplace_back(std::move(work));
        }
    }
}

void Scheduler::allocateWorkerCommandBuffer() {
    current_cmd_buf_ = command_pool_->commit();
    current_cmd_buf_.begin(
        ::vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    current_upload_cmd_buf_ = command_pool_->commit();
    current_upload_cmd_buf_.begin(
        vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
}

void Scheduler::acquireNewChunk() {
    const std::scoped_lock rl{reserve_mutex_};

    if (chunk_reserve_.empty()) {
        // If we don't have anything reserved, we need to make a new chunk.
        chunk_ = std::make_unique<CommandChunk>();
    } else {
        // Otherwise, we can just take from the reserve.
        chunk_ = std::move(chunk_reserve_.back());
        chunk_reserve_.pop_back();
    }
}

void Scheduler::waitWorker() {
    ZoneScopedN("Scheduler::waitWorker()");
    dispatchWork();

    // Ensure the queue is drained.
    {
        std::unique_lock ql{queue_mutex_};
        event_cv_.wait(ql, [this] { return work_queue_.empty(); });
    }

    // Now wait for execution to finish.
    std::scoped_lock el{execution_mutex_};
}

void Scheduler::dispatchWork() {
    if (chunk_->empty()) {
        return;
    }
    {
        std::scoped_lock ql{queue_mutex_};
        work_queue_.push(std::move(chunk_));
    }
    event_cv_.notify_all();
    acquireNewChunk();
}

void Scheduler::endPendingOperations() {
    if (use_dynamic_rendering) {
        endRendering();
    } else {
        endRenderPass();
    }
}
auto Scheduler::updateGraphicsPipeline(GraphicsPipeline* pipeline) -> bool {
    if (state_.graphics_pipeline_ == pipeline) {
        return false;
    }
    state_.graphics_pipeline_ = pipeline;
    return true;
}

void Scheduler::endRendering() {
    if (!dynamic_state.begin_rendering) {
        return;
    }
    record([num_images = num_render_pass_images_, images = render_pass_images_,
            ranges = render_pass_image_ranges_](vk::CommandBuffer cmdbuf) {
        boost::container::small_vector<vk::ImageMemoryBarrier, 9> barriers;
        for (size_t i = 0; i < num_images; ++i) {
            barriers.push_back(
                vk::ImageMemoryBarrier()
                    .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite |
                                      vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                    .setDstAccessMask(vk::AccessFlagBits::eShaderRead |
                                      vk::AccessFlagBits::eShaderWrite |
                                      vk::AccessFlagBits::eColorAttachmentRead |
                                      vk::AccessFlagBits::eColorAttachmentWrite |
                                      vk::AccessFlagBits::eDepthStencilAttachmentRead |
                                      vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                    .setOldLayout(vk::ImageLayout::eGeneral)
                    .setNewLayout(vk::ImageLayout::eGeneral)
                    .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                    .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                    .setImage(images[i])
                    .setSubresourceRange(ranges[i]));
        }
        cmdbuf.endRendering();
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eEarlyFragmentTests |
                                   vk::PipelineStageFlagBits::eLateFragmentTests |
                                   vk::PipelineStageFlagBits::eColorAttachmentOutput,
                               vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, barriers);
    });
    dynamic_state.begin_rendering = false;
    num_render_pass_images_ = 0;
}

void Scheduler::endRenderPass() {
    if (!state_.render_pass_) {
        return;
    }
    record([num_images = num_render_pass_images_, images = render_pass_images_,
            ranges = render_pass_image_ranges_](vk::CommandBuffer cmdbuf) {
        boost::container::small_vector<vk::ImageMemoryBarrier, 9> barriers;
        for (size_t i = 0; i < num_images; ++i) {
            barriers.push_back(
                vk::ImageMemoryBarrier()
                    .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite |
                                      vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                    .setDstAccessMask(vk::AccessFlagBits::eShaderRead |
                                      vk::AccessFlagBits::eShaderWrite |
                                      vk::AccessFlagBits::eColorAttachmentRead |
                                      vk::AccessFlagBits::eColorAttachmentWrite |
                                      vk::AccessFlagBits::eDepthStencilAttachmentRead |
                                      vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                    .setOldLayout(vk::ImageLayout::eGeneral)
                    .setNewLayout(vk::ImageLayout::eGeneral)
                    .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                    .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                    .setImage(images[i])
                    .setSubresourceRange(ranges[i]));
        }
        cmdbuf.endRenderPass();
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eEarlyFragmentTests |
                                   vk::PipelineStageFlagBits::eLateFragmentTests |
                                   vk::PipelineStageFlagBits::eColorAttachmentOutput,
                               vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, barriers);
    });
    state_.render_pass_ = nullptr;
    num_render_pass_images_ = 0;
}

void Scheduler::invalidateState() { state_.graphics_pipeline_ = nullptr; }

/**
 * @brief 提交一个执行任务到调度器。
 *
 * 该函数提交一个执行任务到调度器，并返回信号量的值。它首先结束所有挂起的操作，
 * 然后使当前状态无效。接着，它获取下一个信号量的值，并使用上传缓冲区记录命令。
 * 最后，它提交命令队列并调度工作。
 *
 * @param signal_semaphore 用于信号的 Vulkan 信号量。
 * @param wait_semaphore 用于等待的 Vulkan 信号量。
 * @return u64 返回信号量的值。
 */
auto Scheduler::submitExecution(vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore)
    -> u64 {
    endPendingOperations();
    invalidateState();

    const u64 signal_value = master_semaphore_->nextTick();
    recordWithUploadBuffer([signal_semaphore, wait_semaphore, signal_value, this](
                               vk::CommandBuffer cmdbuf, vk::CommandBuffer upload_cmdbuf) {
        static constexpr vk::MemoryBarrier WRITE_BARRIER =
            vk::MemoryBarrier()
                .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eMemoryWrite |
                                  vk::AccessFlagBits::eMemoryRead);
        upload_cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eAllCommands, {}, WRITE_BARRIER,
                                      {}, {});
        upload_cmdbuf.end();
        cmdbuf.end();
        std::scoped_lock lock{submit_mutex_};
        master_semaphore_->submitQueue(cmdbuf, upload_cmdbuf, signal_semaphore, wait_semaphore,
                                       signal_value);
    });
    chunk_->markSubmit();
    dispatchWork();
    return signal_value;
}

auto Scheduler::flush(vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore) -> u64 {
    // When flushing, we only send data to the worker thread; no waiting is necessary.
    const u64 signal_value = submitExecution(signal_semaphore, wait_semaphore);
    return signal_value;
}
void Scheduler::finish(vk::Semaphore signal_semaphore, VkSemaphore wait_semaphore) {
    // When finishing, we need to wait for the submission to have executed on the device.
    const u64 presubmit_tick = currentTick();
    submitExecution(signal_semaphore, wait_semaphore);
    wait(presubmit_tick);
}

void Scheduler::requestRenderPass(const TextureFramebuffer* framebuffer) {
    const vk::RenderPass render_pass = framebuffer->RenderPass();
    const vk::Framebuffer framebuffer_handle = framebuffer->Handle();
    const VkExtent2D render_area = framebuffer->RenderArea();
    if (render_pass == state_.render_pass_ && framebuffer_handle == state_.framebuffer_ &&
        render_area.width == state_.render_area_.width &&
        render_area.height == state_.render_area_.height) {
        return;
    }
    endRenderPass();
    state_.render_pass_ = render_pass;
    state_.framebuffer_ = framebuffer_handle;
    state_.render_area_ = render_area;

    record([render_pass, framebuffer_handle, render_area](vk::CommandBuffer cmdbuf) {
        cmdbuf.beginRenderPass(
            vk::RenderPassBeginInfo()
                .setRenderPass(render_pass)
                .setFramebuffer(framebuffer_handle)
                .setRenderArea(
                    vk::Rect2D().setOffset(vk::Offset2D().setX(0).setY(0)).setExtent(render_area)),
            vk::SubpassContents::eInline);
    });
    num_render_pass_images_ = framebuffer->NumImages();
    render_pass_images_ = framebuffer->Images();
    render_pass_image_ranges_ = framebuffer->ImageRanges();
}

void Scheduler::requestRendering(const TextureFramebuffer* framebuffer) {
    if (dynamic_state.begin_rendering && framebuffer->DepthView() == dynamic_state.depth_view &&
        framebuffer->RenderArea() == dynamic_state.render_area_ &&
        dynamic_state.color_views == framebuffer->ImageViews()) {
        return;
    }
    endRendering();
    dynamic_state.color_views = framebuffer->ImageViews();
    dynamic_state.depth_view = framebuffer->DepthView();
    dynamic_state.render_area_ = framebuffer->RenderArea();
    dynamic_state.begin_rendering = true;

    record([this](vk::CommandBuffer cmdbuf) {
        boost::container::small_vector<vk::RenderingAttachmentInfo, 8> colorAttachments;
        for (const auto& view : dynamic_state.color_views) {
            if (view) {
                colorAttachments.push_back(
                    vk::RenderingAttachmentInfo()
                        .setImageView(view)
                        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                        .setLoadOp(vk::AttachmentLoadOp::eLoad)
                        .setStoreOp(vk::AttachmentStoreOp::eStore)
                        .setClearValue(vk::ClearValue().setColor({1.0f, 1.0f, 1.0f, 1.0f})));
            }
        }

        vk::RenderingInfo renderingInfo =
            vk::RenderingInfo()
                .setLayerCount(1)
                .setRenderArea(vk::Rect2D().setExtent(dynamic_state.render_area_))
                .setColorAttachments(colorAttachments);
        vk::RenderingAttachmentInfo depthAttachment;
        if (dynamic_state.depth_view) {
            depthAttachment = vk::RenderingAttachmentInfo()
                                  .setImageView(dynamic_state.depth_view)
                                  .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                                  .setLoadOp(vk::AttachmentLoadOp::eLoad)
                                  .setStoreOp(vk::AttachmentStoreOp::eStore)
                                  .setClearValue(vk::ClearValue().setDepthStencil(
                                      vk::ClearDepthStencilValue().setDepth(1.f)));
            depthAttachment.sType = vk::StructureType::eRenderingAttachmentInfo;
            renderingInfo.setPDepthAttachment(&depthAttachment);
        }
        cmdbuf.beginRendering(&renderingInfo);
    });
    num_render_pass_images_ = framebuffer->NumImages();
    render_pass_images_ = framebuffer->Images();
    render_pass_image_ranges_ = framebuffer->ImageRanges();
}

void Scheduler::requestRender(const TextureFramebuffer* framebuffer) {
    if (use_dynamic_rendering) {
        requestRendering(framebuffer);
    } else {
        requestRenderPass(framebuffer);
    }
}

void Scheduler::requestOutsideRenderOperationContext() {
    if (use_dynamic_rendering) {
        endRendering();
    } else {
        endRenderPass();
    }
}

}  // namespace render::vulkan::scheduler