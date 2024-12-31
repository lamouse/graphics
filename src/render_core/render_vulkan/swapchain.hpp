#pragma once
#include <vulkan/vulkan.hpp>
#include "vulkan_common/vulkan_common.hpp"
namespace render::vulkan {
class Device;
namespace scheduler {
class Scheduler;
}
class Swapchain {
    public:
        explicit Swapchain(vk::SurfaceKHR surface, const Device& device, scheduler::Scheduler& scheduler,
                           uint32_t width, uint32_t height);
        ~Swapchain() = default;

        /// Creates (or recreates) the swapchain with a given size.
        void create(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
        /// Acquires the next image in the swapchain, waits as needed.
        auto acquireNextImage() -> bool;
        static constexpr ::vk::Format DEFAULT_COLOR_FORMAT = ::vk::Format::eB8G8R8A8Unorm;
        static constexpr ::vk::ColorSpaceKHR DEFAULT_COLOR_SPACE = ::vk::ColorSpaceKHR::eSrgbNonlinear;

    private:
        vk::SurfaceKHR surface_;
        const Device& device_;
        scheduler::Scheduler& scheduler_;
        vk::SwapchainKHR swapchain_;

        std::size_t image_count_{};
        std::vector<vk::Image> images_;
        std::vector<uint64_t> resource_ticks_;
        std::vector<vk::Semaphore> present_semaphores_;
        std::vector<vk::Semaphore> render_semaphores_;

        uint32_t width_;
        uint32_t height_;

        uint32_t image_index_{};
        uint32_t frame_index_{};

        vk::Format image_view_format_{};
        vk::Extent2D extent_;
        vk::PresentModeKHR present_mode_{};
        vk::SurfaceFormatKHR surface_format_;

        bool is_outdated_{};
        bool is_suboptimal_{};

        bool has_imm_{false};
        bool has_mailbox_{false};
        bool has_fifo_relaxed_{false};

        void destroy();
        void createSwapchain(const vk::SurfaceCapabilitiesKHR& capabilities);
        void init_sync_mode();
        void createSemaphores();
};
}  // namespace render::vulkan