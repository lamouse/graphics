#pragma once
#include <stop_token>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "render_core/vulkan_common/memory_allocator.hpp"
#include "common/common_funcs.hpp"
import render.vulkan.common;
namespace core::frontend {
class BaseWindow;
}
namespace render::vulkan {

class Swapchain;
namespace scheduler {
class Scheduler;
}
struct Frame {
        uint32_t width;
        uint32_t height;
        Image image;
        ImageView image_view;
        vk::CommandBuffer cmdbuf;
        Semaphore render_ready;
        Fence present_done;
};

class PresentManager {
    public:
        PresentManager(const vk::Instance& instance, core::frontend::BaseWindow& render_window,
                       const Device& device, MemoryAllocator& memory_allocator,
                       scheduler::Scheduler& scheduler, Swapchain& swapchain, SurfaceKHR& surface);
        CLASS_NON_COPYABLE(PresentManager);
        CLASS_NON_MOVEABLE(PresentManager);
        ~PresentManager() = default;
        /// Returns the last used presentation frame
        auto getRenderFrame() -> Frame*;

        /// Pushes a frame for presentation
        void present(Frame* frame);

        /// Recreates the present frame to match the provided parameters
        void recreateFrame(Frame* frame, uint32_t width, uint32_t height,
                           vk::Format image_view_format);

        /// Waits for the present thread to finish presenting all queued frames.
        void waitPresent();

    private:
        void presentThread(std::stop_token token);

        void copyToSwapchain(Frame* frame);

        void copyToSwapchainImpl(Frame* frame);

        void recreateSwapchain(Frame* frame);

        void setImageCount();

        const vk::Instance instance_;
        core::frontend::BaseWindow& render_window_;
        const Device& device_;
        MemoryAllocator& memory_allocator_;
        scheduler::Scheduler& scheduler_;
        Swapchain& swapchain_;
        SurfaceKHR& surface_;
        VulkanCommandPool cmdpool_;
        std::vector<Frame> frames_;
        std::queue<Frame*> present_queue_;
        std::queue<Frame*> free_queue_;
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