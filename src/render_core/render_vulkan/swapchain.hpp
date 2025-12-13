#pragma once
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
namespace render::vulkan {
class Device;
namespace scheduler {
class Scheduler;
}
class Swapchain {
    public:
        explicit Swapchain(vk::SurfaceKHR surface, const Device& device,
                           scheduler::Scheduler& scheduler, uint32_t width, uint32_t height);
        ~Swapchain() = default;

        /// Creates (or recreates) the swapchain with a given size.
        void create(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
        /// Acquires the next image in the swapchain, waits as needed.
        auto acquireNextImage() -> bool;
        void present(vk::Semaphore render_semaphore);
        /// Returns true when the swapchain needs to be recreated.
        [[nodiscard]] auto needsRecreation() const -> bool {
            return isSubOptimal() || needsPresentModeUpdate();
        }
        /// Returns true when the swapchain is outdated.
        [[nodiscard]] auto isOutDated() const -> bool { return is_outdated_; }

        /// Returns true when the swapchain is suboptimal.
        [[nodiscard]] auto isSubOptimal() const -> bool { return is_suboptimal_; }

        [[nodiscard]] auto getSize() const -> vk::Extent2D { return extent_; }
        [[nodiscard]] auto getImageCount() const -> std::size_t { return image_count_; }

        [[nodiscard]] auto getImageIndex() const -> std::size_t { return image_index_; }

        [[nodiscard]] auto getFrameIndex() const -> std::size_t { return frame_index_; }
        [[nodiscard]] auto getImageIndex(std::size_t index) const -> VkImage {
            return images_[index];
        }

        [[nodiscard]] auto currentImage() const -> vk::Image { return images_[image_index_]; }

        [[nodiscard]] auto getImageViewFormat() const -> vk::Format { return image_view_format_; }
        [[nodiscard]] auto currentPresentSemaphore() const -> vk::Semaphore {
            return *present_semaphores_[frame_index_];
        }

        [[nodiscard]] auto currentRenderSemaphore() const -> vk::Semaphore {
            return *render_semaphores_[frame_index_];
        }

        [[nodiscard]] auto getWidth() const { return width_; }

        [[nodiscard]] auto getHeight() const { return height_; }

        [[nodiscard]] auto getExtent() const -> vk::Extent2D { return extent_; }

        [[nodiscard]] auto getImageFormat() const -> vk::Format { return surface_format_.format; }
        static constexpr ::vk::Format DEFAULT_COLOR_FORMAT = ::vk::Format::eB8G8R8A8Unorm;
        static constexpr ::vk::ColorSpaceKHR DEFAULT_COLOR_SPACE =
            ::vk::ColorSpaceKHR::eSrgbNonlinear;

    private:
        vk::SurfaceKHR surface_;
        const Device& device_;
        scheduler::Scheduler& scheduler_;
        SwapchainKHR swapchain_;

        std::size_t image_count_{};
        std::vector<vk::Image> images_;
        std::vector<uint64_t> resource_ticks_;
        std::vector<Semaphore> present_semaphores_;
        std::vector<Semaphore> render_semaphores_;

        uint32_t width_{};
        uint32_t height_{};

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
        [[nodiscard]] auto needsPresentModeUpdate() const -> bool;
};
}  // namespace render::vulkan