#include "scheduler.hpp"
#include "master_semaphore.hpp"
#include "command_pool.hpp"
#include "common/polyfill_thread.hpp"
#include "common/microprofile.hpp"
#include "vulkan_common/device.hpp"
#include "texture_cache.hpp"

namespace render::vulkan::scheduler {
// MICROPROFILE_DECLARE(Vulkan_WaitForWorker);

void Scheduler::CommandChunk::executeAll(vk::CommandBuffer cmdbuf,
                                         vk::CommandBuffer upload_cmdbuf) {
    auto command = first;
    while (command != nullptr) {
        auto* next = gsl::owner<Command*>(command->getNext());
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
      command_pool_{std::make_unique<resource::CommandPool>(master_semaphore_.get(), device)} {
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
    ::vk::CommandBufferBeginInfo begin{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    current_cmd_buf_.begin(begin);
    current_upload_cmd_buf_ = command_pool_->commit();
    ::vk::CommandBufferBeginInfo current_begin{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    current_upload_cmd_buf_.begin(current_begin);
}

void Scheduler::acquireNewChunk() {
    std::scoped_lock rl{reserve_mutex_};

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
    // MICROPROFILE_SCOPE(Vulkan_WaitForWorker);
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
#if ANDROID
    if (Settings::IsGPULevelHigh()) {
        // This is problematic on Android, disable on GPU Normal.
        // query_cache->DisableStreams();
    }
#else
    // query_cache->DisableStreams();
#endif
    endRenderPass();
}

void Scheduler::endRenderPass() {
    if (!state_.render_pass_) {
        return;
    }
    record([num_images = num_render_pass_images_, images = render_pass_images_,
            ranges = render_pass_image_ranges_](vk::CommandBuffer cmdbuf) {
        std::array<vk::ImageMemoryBarrier, 9> barriers;
        for (size_t i = 0; i < num_images; ++i) {
            barriers[i] = vk::ImageMemoryBarrier{
                vk::AccessFlagBits::eColorAttachmentWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite |
                    vk::AccessFlagBits::eColorAttachmentRead |
                    vk::AccessFlagBits::eColorAttachmentWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentRead |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eGeneral,

                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                images[i],
                ranges[i],
            };
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

void Scheduler::invalidateState() {
    state_.graphics_pipeline_ = nullptr;
    state_.rescaling_defined_ = false;
}
#undef MemoryBarrier
u64 Scheduler::submitExecution(vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore) {
    endPendingOperations();
    invalidateState();

    const u64 signal_value = master_semaphore_->nextTick();
    recordWithUploadBuffer([signal_semaphore, wait_semaphore, signal_value, this](
                               vk::CommandBuffer cmdbuf, vk::CommandBuffer upload_cmdbuf) {
        static constexpr vk::MemoryBarrier WRITE_BARRIER{
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead};
        upload_cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eAllCommands, {}, WRITE_BARRIER,
                                      {}, {});
        upload_cmdbuf.end();
        cmdbuf.end();

        if (on_submit_) {
            on_submit_();
        }

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

void Scheduler::requestRenderpass(const TextureFramebuffer* framebuffer) {
    const vk::RenderPass renderpass = framebuffer->RenderPass();
    const vk::Framebuffer framebuffer_handle = framebuffer->Handle();
    const VkExtent2D render_area = framebuffer->RenderArea();
    if (renderpass == state_.render_pass_ && framebuffer_handle == state_.framebuffer_ &&
        render_area.width == state_.render_area_.width &&
        render_area.height == state_.render_area_.height) {
        return;
    }
    endRenderPass();
    state_.render_pass_ = renderpass;
    state_.framebuffer_ = framebuffer_handle;
    state_.render_area_ = render_area;

    record([renderpass, framebuffer_handle, render_area](vk::CommandBuffer cmdbuf) {
        const VkRenderPassBeginInfo renderpass_bi{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = renderpass,
            .framebuffer = framebuffer_handle,
            .renderArea =
                {
                    .offset = {.x = 0, .y = 0},
                    .extent = render_area,
                },
            .clearValueCount = 0,
            .pClearValues = nullptr,
        };
        cmdbuf.beginRenderPass(renderpass_bi, vk::SubpassContents::eInline);
    });
    num_render_pass_images_ = framebuffer->NumImages();
    render_pass_images_ = framebuffer->Images();
    render_pass_image_ranges_ = framebuffer->ImageRanges();
}

}  // namespace render::vulkan::scheduler