#include "scheduler.hpp"
#include "master_semaphore.hpp"
#include "command_pool.hpp"
#include "common/polyfill_thread.hpp"
#include "command_pool.hpp"
namespace render::vulkan::scheduler {
void Scheduler::CommandChunk::executeAll(vk::CommandBuffer cmdbuf, vk::CommandBuffer upload_cmdbuf) {
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

Scheduler::Scheduler(const Device& device, StateTracker& state_tracker)
    : device_{device},
      state_tracker_{state_tracker},
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
            common::thread::condvarWait(event_cv_, lk, stop_token, [&] { return TryPopQueue(work); });

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

}  // namespace render::vulkan::scheduler