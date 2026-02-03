#include "master_semaphore.hpp"
#include "vulkan_common/device.hpp"
#include <algorithm>
#include <utility>
#include "common/polyfill_thread.hpp"
#include <spdlog/spdlog.h>
#include "common/settings.hpp"
namespace render::vulkan::semaphore {
constexpr uint64_t FENCE_RESERVE_SIZE = 8;
MasterSemaphore::MasterSemaphore(const Device& device) : device_(device) {
    if (!device.hasTimelineSemaphore()) {
        static constexpr vk::FenceCreateInfo fence_ci;
        free_queue_.resize(FENCE_RESERVE_SIZE);
        std::ranges::generate(free_queue_, [&] { return device.logical().createFence(fence_ci); });
        wait_thread_ =
            std::jthread([this](std::stop_token token) { waitThread(std::move(token)); });
        return;
    }

    static constexpr vk::SemaphoreTypeCreateInfo semaphore_type_ci =
        vk::SemaphoreTypeCreateInfo()
            .setSemaphoreType(vk::SemaphoreType::eTimeline)
            .setInitialValue(0);

    static constexpr vk::SemaphoreCreateInfo semaphore_ci =
        vk::SemaphoreCreateInfo().setPNext(&semaphore_type_ci);
    semaphore_ = device.logical().CreateSemaphore(semaphore_ci);

    if (not settings::values.render_debug.GetValue()) {
        return;
    }
    // Validation layers have a bug where they fail to track resource usage when using timeline
    // semaphores and synchronizing with GetSemaphoreCounterValue. To workaround this issue, have
    // a separate thread waiting for each timeline semaphore value.
    // NOLINTNEXTLINE
    debug_thread_ = std::jthread([this](std::stop_token stop_token) {
        u64 counter = 0;
        while (!stop_token.stop_requested()) {
            if (semaphore_.Wait(counter, 10'000'000)) {
                ++counter;
            }
        }
    });
}
// NOLINTNEXTLINE
void MasterSemaphore::waitThread(std::stop_token token) {
    while (!token.stop_requested()) {
        uint64_t host_tick{};
        Fence fence;
        {
            std::unique_lock lock{wait_mutex_};
            common::thread::condvarWait(wait_cv_, lock, token,
                                        [this] { return !wait_queue_.empty(); });
            if (token.stop_requested()) {
                return;
            }
            std::tie(host_tick, fence) = std::move(wait_queue_.front());
            wait_queue_.pop();
        }

        auto waitResult = fence.Wait();
        if (waitResult != ::vk::Result::eSuccess) {
            throw ::std::runtime_error("wait fences");
        }
        fence.Reset();

        {
            std::scoped_lock lock{free_mutex_};
            free_queue_.push_front(std::move(fence));
            gpu_tick_.store(host_tick);
        }
        free_cv_.notify_one();
    }
}

auto MasterSemaphore::getFreeFence() -> Fence {
    std::scoped_lock lock{free_mutex_};
    if (free_queue_.empty()) {
        static constexpr VkFenceCreateInfo fence_ci{};
        return device_.logical().createFence(fence_ci);
    }

    auto fence = std::move(free_queue_.back());
    free_queue_.pop_back();
    return fence;
}
static constexpr std::array<vk::PipelineStageFlags, 2> wait_stage_masks{
    ::vk::PipelineStageFlagBits::eAllCommands, ::vk::PipelineStageFlagBits::eColorAttachmentOutput};

void MasterSemaphore::submitQueueFence(vk::CommandBuffer& cmdbuf, vk::CommandBuffer& upload_cmdbuf,
                                       vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore,
                                       uint64_t host_tick) {
    SPDLOG_DEBUG("MasterSemaphore::submitQueueFence host tick {}", host_tick);
    const uint32_t num_signal_semaphores = signal_semaphore ? 1 : 0;
    const uint32_t num_wait_semaphores = wait_semaphore ? 1 : 0;

    const std::array cmdbuffers{upload_cmdbuf, cmdbuf};

    vk::SubmitInfo submit_info = vk::SubmitInfo()
                                     .setWaitSemaphoreCount(num_wait_semaphores)
                                     .setWaitSemaphores(wait_semaphore)
                                     .setWaitDstStageMask(wait_stage_masks)
                                     .setCommandBuffers(cmdbuffers)
                                     .setSignalSemaphoreCount(num_signal_semaphores)
                                     .setSignalSemaphores(signal_semaphore);

    auto fence = getFreeFence();
    try {
        device_.getGraphicsQueue().submit(submit_info, *fence);
        std::scoped_lock lock{wait_mutex_};
        wait_queue_.emplace(host_tick, std::move(fence));
        wait_cv_.notify_one();
    } catch (std::exception& e) {
        SPDLOG_ERROR("GraphicsQueue submit error: {}", e.what());
        std::scoped_lock lock{wait_mutex_};
        wait_queue_.emplace(host_tick, std::move(fence));
        wait_cv_.notify_one();
        throw e;
    }
}

void MasterSemaphore::submitQueueTimeline(vk::CommandBuffer& cmdbuf,
                                          vk::CommandBuffer& upload_cmdbuf,
                                          vk::Semaphore signal_semaphore,
                                          vk::Semaphore wait_semaphore, uint64_t host_tick) {
    const vk::Semaphore timeline_semaphore = *semaphore_;

    std::vector<uint64_t> signal_values{host_tick};
    std::vector<vk::Semaphore> signal_semaphores = {timeline_semaphore};
    if (signal_semaphore) {
        signal_values.push_back(uint64_t(0));
        signal_semaphores.push_back(signal_semaphore);
    }
    const std::array cmdbuffers{upload_cmdbuf, cmdbuf};
    const vk::TimelineSemaphoreSubmitInfo timeline_si =
        vk::TimelineSemaphoreSubmitInfo().setSignalSemaphoreValues(signal_values);
    vk::SubmitInfo submit_info = vk::SubmitInfo()
                                     .setPNext(&timeline_si)
                                     .setWaitDstStageMask(wait_stage_masks)
                                     .setCommandBuffers(cmdbuffers)
                                     .setSignalSemaphores(signal_semaphores);
    if (wait_semaphore) {
        submit_info.setWaitSemaphores(wait_semaphore);
    } else {
        submit_info.setWaitSemaphoreCount(0);
    }
    try {
        device_.getGraphicsQueue().submit(submit_info);
    } catch (std::exception& e) {
        SPDLOG_ERROR("GraphicsQueue submit error: {}", e.what());
    }
}
/**
 * @brief 刷新时间线信号量的计数器。
 *
 * 该函数用于刷新 GPU 时间线信号量的计数器。它首先检查是否支持时间线信号量，
 * 如果不支持，则直接返回。然后，它获取当前的 GPU 计数器值，并与时间线信号量的计数器值进行比较。
 * 如果时间线信号量的计数器值小于当前的 GPU 计数器值，则直接返回。
 * 否则，它会更新 GPU 计数器值，直到成功为止。
 */
void MasterSemaphore::refresh() {
    if (!semaphore_) {
        // If we don't support timeline semaphores, there's nothing to refresh
        return;
    }
    uint64_t this_tick{};
    uint64_t counter{};
    do {
        this_tick = gpu_tick_.load(std::memory_order_acquire);
        counter = semaphore_.GetCounter();
        if (counter < this_tick) {
            return;
        }
    } while (!gpu_tick_.compare_exchange_weak(this_tick, counter, std::memory_order_release,
                                              std::memory_order_relaxed));
}

void MasterSemaphore::wait(u64 tick) {
    if (!semaphore_) {
        // If we don't support timeline semaphores, wait for the value normally
        std::unique_lock lk{free_mutex_};
        free_cv_.wait(lk, [&] { return gpu_tick_.load(std::memory_order_relaxed) >= tick; });
        return;
    }

    // No need to wait if the GPU is ahead of the tick
    if (isFree(tick)) {
        return;
    }

    // Update the GPU tick and try again
    refresh();

    if (isFree(tick)) {
        return;
    }

    // If none of the above is hit, fallback to a regular wait
    while (!semaphore_.Wait(tick)) {
    }

    refresh();
}

void MasterSemaphore::submitQueue(vk::CommandBuffer& cmdbuf, vk::CommandBuffer& upload_cmdbuf,
                                  vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore,
                                  u64 host_tick) {
    if (semaphore_) {
        return submitQueueTimeline(cmdbuf, upload_cmdbuf, signal_semaphore, wait_semaphore,
                                   host_tick);
    }
    return submitQueueFence(cmdbuf, upload_cmdbuf, signal_semaphore, wait_semaphore, host_tick);
}

}  // namespace render::vulkan::semaphore