#pragma once
#include <stop_token>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include "render_core/vulkan_common/memory_allocator.hpp"
#include "render_core/render_vulkan/present/present_frame.hpp"

namespace core::frontend {
class BaseWindow;
}
namespace render::vulkan {
class Device;
class Swapchain;
namespace scheduler {
class Scheduler;
}

class PresentManager {
    public:
        PresentManager(const vk::Instance& instance, core::frontend::BaseWindow& render_window,
                       const Device& device, MemoryAllocator& memory_allocator,
                       scheduler::Scheduler& scheduler, Swapchain& swapchain, SurfaceKHR& surface);
        PresentManager(const PresentManager&) = delete;
        PresentManager(PresentManager&&) = delete;
        auto operator=(const PresentManager&)-> PresentManager& = delete;
        auto operator=(PresentManager&&) noexcept -> PresentManager& = delete;

        ~PresentManager() = default;
        /// Returns the last used presentation frame
        auto getRenderFrame() -> present::Frame*;

        /// Pushes a frame for presentation
        void present(present::Frame* frame);

        /// Recreates the present frame to match the provided parameters
        void recreateFrame(present::Frame* frame, uint32_t width, uint32_t height,
                           vk::Format image_view_format);

        /// Waits for the present thread to finish presenting all queued frames.
        void waitPresent();

    private:
        void presentThread(std::stop_token token);

        void copyToSwapchain(present::Frame* frame);

        void copyToSwapchainImpl(present::Frame* frame);

        void recreateSwapchain(present::Frame* frame);

        void setImageCount();

        const vk::Instance instance_;
        core::frontend::BaseWindow& render_window_;
        const Device& device_;
        MemoryAllocator& memory_allocator_;
        scheduler::Scheduler& scheduler_;
        Swapchain& swapchain_;
        SurfaceKHR& surface_;
        VulkanCommandPool cmdpool_;
        std::vector<present::Frame> frames_;
        std::queue<present::Frame*> present_queue_;
        std::queue<present::Frame*> free_queue_;
        std::condition_variable_any frame_cv_;
        std::condition_variable free_cv_;
        std::mutex swapchain_mutex_;
        std::mutex queue_mutex_;
        std::mutex free_mutex_;
        std::jthread present_thread_;
        bool blit_supported_;
        bool use_present_thread_;
        std::size_t image_count_{};
};

}  // namespace render::vulkan