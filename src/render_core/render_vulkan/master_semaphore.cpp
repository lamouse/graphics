#include "master_semaphore.hpp"
#include "vulkan_common/device.hpp"
#include <algorithm>
#include <utility>
#include "common/polyfill_thread.hpp"
#include <spdlog/spdlog.h>
namespace render::vulkan::semaphore {
constexpr uint64_t FENCE_RESERVE_SIZE = 8;
MasterSemaphore::MasterSemaphore(const Device& device) : device_(device) {
    if (!device.hasTimelineSemaphore()) {
        static constexpr vk::FenceCreateInfo fence_ci;
        free_queue_.resize(FENCE_RESERVE_SIZE);
        std::ranges::generate(free_queue_, [&] { return device.getLogical().createFence(fence_ci); });
        wait_thread_ = std::jthread([this](std::stop_token token) { waitThread(std::move(token)); });
        return;
    }
}
void MasterSemaphore::waitThread(std::stop_token token) {
    while (!token.stop_requested()) {
        uint64_t host_tick{};
        vk::Fence fence;
        {
            std::unique_lock lock{wait_mutex_};
            common::thread::condvarWait(wait_cv_, lock, token, [this] { return !wait_queue_.empty(); });
            if (token.stop_requested()) {
                return;
            }
            std::tie(host_tick, fence) = wait_queue_.front();
            wait_queue_.pop();
        }

        auto waitResult = device_.getLogical().waitForFences(fence, VK_TRUE, ::std::numeric_limits<uint64_t>::max());
        if (waitResult != ::vk::Result::eSuccess) {
            throw ::std::runtime_error("wait fences");
        }
        device_.getLogical().resetFences(fence);

        {
            std::scoped_lock lock{free_mutex_};
            free_queue_.push_front(fence);
            gpu_tick_.store(host_tick);
        }
        free_cv_.notify_one();
    }
}

auto MasterSemaphore::getFreeFence() -> vk::Fence {
    std::scoped_lock lock{free_mutex_};
    if (free_queue_.empty()) {
        static constexpr VkFenceCreateInfo fence_ci{};
        return device_.getLogical().createFence(fence_ci);
    }

    auto fence = free_queue_.back();
    free_queue_.pop_back();
    return fence;
}
static constexpr std::array<vk::PipelineStageFlags, 2> wait_stage_masks{
    ::vk::PipelineStageFlagBits::eAllCommands, ::vk::PipelineStageFlagBits::eColorAttachmentOutput};

void MasterSemaphore::submitQueueFence(vk::CommandBuffer& cmdbuf, vk::CommandBuffer& upload_cmdbuf,
                                       vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore,
                                       uint64_t host_tick) {
    const uint32_t num_signal_semaphores = signal_semaphore ? 1 : 0;
    const uint32_t num_wait_semaphores = wait_semaphore ? 1 : 0;

    const std::array cmdbuffers{upload_cmdbuf, cmdbuf};

    vk::SubmitInfo submit_info{};
    submit_info.setWaitSemaphoreCount(num_wait_semaphores)
        .setWaitSemaphores(wait_semaphore)
        .setWaitDstStageMask(wait_stage_masks)
        .setCommandBuffers(cmdbuffers)
        .setSignalSemaphoreCount(num_signal_semaphores)
        .setSignalSemaphores(signal_semaphore);

    auto fence = getFreeFence();
    try {
        device_.getGraphicsQueue().submit(submit_info, fence);
        std::scoped_lock lock{wait_mutex_};
        wait_queue_.emplace(host_tick, fence);
        wait_cv_.notify_one();
    } catch (std::exception& e) {
        SPDLOG_ERROR("GraphicsQueue submit error: {}", e.what());
        std::scoped_lock lock{wait_mutex_};
        wait_queue_.emplace(host_tick, fence);
        wait_cv_.notify_one();
        throw e;
    }
}

void MasterSemaphore::submitQueueTimeline(vk::CommandBuffer& cmdbuf, vk::CommandBuffer& upload_cmdbuf,
                                          vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore,
                                          uint64_t host_tick) {
    const vk::Semaphore timeline_semaphore = semaphore_;

    const std::array signal_values{host_tick, uint64_t(0)};
    const std::array signal_semaphores{timeline_semaphore, signal_semaphore};

    const std::array cmdbuffers{upload_cmdbuf, cmdbuf};

    const uint32_t num_wait_semaphores = wait_semaphore ? 1 : 0;
    vk::TimelineSemaphoreSubmitInfo timeline_si;
    timeline_si.setSignalSemaphoreValues(signal_values);

    vk::SubmitInfo submit_info{};
    submit_info.setPNext(&timeline_si)
        .setWaitSemaphoreCount(num_wait_semaphores)
        .setWaitSemaphores(wait_semaphore)
        .setWaitDstStageMask(wait_stage_masks)
        .setCommandBuffers(cmdbuffers)
        .setSignalSemaphores(signal_semaphores);

    try {
        device_.getGraphicsQueue().submit(submit_info);
    } catch (std::exception& e) {
        SPDLOG_ERROR("GraphicsQueue submit error: {}", e.what());
    }
}

void MasterSemaphore::refresh() {
    if (!semaphore_) {
        // If we don't support timeline semaphores, there's nothing to refresh
        return;
    }

    uint64_t this_tick{};
    uint64_t counter{};
    do {
        this_tick = gpu_tick_.load(std::memory_order_acquire);
        counter = device_.getLogical().getSemaphoreCounterValue(semaphore_);
        if (counter < this_tick) {
            return;
        }
    } while (
        !gpu_tick_.compare_exchange_weak(this_tick, counter, std::memory_order_release, std::memory_order_relaxed));
}

}  // namespace render::vulkan::semaphore