#pragma once
#include <vulkan/vulkan.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "common/common_funcs.hpp"
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
namespace render::vulkan {
class Device;
namespace semaphore {
class MasterSemaphore {
        using Waitable = std::pair<uint64_t, Fence>;

    public:
        CLASS_NON_COPYABLE(MasterSemaphore);
        CLASS_NON_MOVEABLE(MasterSemaphore);
        explicit MasterSemaphore(const Device& device);
        ~MasterSemaphore() = default;
        /// Returns the current logical tick.
        [[nodiscard]] auto currentTick() const noexcept -> uint64_t {
            return current_tick_.load(std::memory_order_acquire);
        }

        /// Returns the last known GPU tick.
        [[nodiscard]] auto knownGpuTick() const noexcept -> uint64_t {
            return gpu_tick_.load(std::memory_order_acquire);
        }

        /// Returns true when a tick has been hit by the GPU.
        [[nodiscard]] auto isFree(uint64_t tick) const noexcept -> bool {
            return knownGpuTick() >= tick;
        }

        /// Advance to the logical tick and return the old one
        [[nodiscard]] auto nextTick() noexcept -> uint64_t {
            return current_tick_.fetch_add(1, std::memory_order_release);
        }

        /// Refresh the known GPU tick
        void refresh();

        /// Waits for a tick to be hit on the GPU
        void wait(uint64_t tick);

        /// Submits the device graphics queue, updating the tick as necessary
        void submitQueue(vk::CommandBuffer& cmdbuf, vk::CommandBuffer& upload_cmdbuf,
                         vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore,
                         uint64_t host_tick);

    private:
        void submitQueueTimeline(vk::CommandBuffer& cmdbuf, vk::CommandBuffer& upload_cmdbuf,
                                 vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore,
                                 uint64_t host_tick);
        void submitQueueFence(vk::CommandBuffer& cmdbuf, vk::CommandBuffer& upload_cmdbuf,
                              vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore,
                              uint64_t host_tick);

        void waitThread(std::stop_token token);

        auto getFreeFence() -> Fence;

        const Device& device_;                   ///< Device.
        Semaphore semaphore_;                    ///< Timeline semaphore.
        std::atomic<uint64_t> gpu_tick_{0};      ///< Current known GPU tick.
        std::atomic<uint64_t> current_tick_{1};  ///< Current logical tick.
        std::mutex wait_mutex_;
        std::mutex free_mutex_;
        std::condition_variable free_cv_;
        std::condition_variable_any wait_cv_;
        std::queue<Waitable>
            wait_queue_;  ///< Queue for the fences to be waited on by the wait thread.
        std::deque<Fence> free_queue_;  ///< Holds available fences for submission.
        std::jthread debug_thread_;     ///< Debug thread to workaround validation layer bugs.
        std::jthread wait_thread_;      ///< Helper thread that waits for submitted fences.
};
}  // namespace semaphore
}  // namespace render::vulkan