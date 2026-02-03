#pragma once

#include "common/common_types.hpp"
#include "common/settings_enums.hpp"
#include "render_core/framebuffer_config.hpp"
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include "render_core/render_vulkan/present/present_frame.hpp"
#include <list>
#include <memory>
#include "core/frontend/framebuffer_layout.hpp"
#include <functional>

namespace render::vulkan {
class Device;
class PresentManager;
class MemoryAllocator;
class Layer;
class Framebuffer;
namespace present {
class WindowAdaptPass;
}

namespace scheduler {
class Scheduler;
}

class BlitScreen {
    public:
        explicit BlitScreen(const Device& device, MemoryAllocator& memory_allocator,
                            PresentManager& present_manager, scheduler::Scheduler& scheduler);
        ~BlitScreen();
        BlitScreen(const BlitScreen&) = delete;
        auto operator=(const BlitScreen&) -> BlitScreen& = delete;
        BlitScreen(BlitScreen&&) noexcept = delete;
        auto operator=(BlitScreen&&) noexcept -> BlitScreen& = delete;

        void DrawToFrame(
            const std::function<std::optional<present::FramebufferTextureInfo>(
                const frame::FramebufferConfig& framebuffer, uint32_t stride)>& accelerateDisplay,
            present::Frame* frame, const layout::FrameBufferLayout& layout,
            std::span<const frame::FramebufferConfig> framebuffers,
            size_t current_swapchain_image_count, vk::Format current_swapchain_view_format);

    private:
        void WaitIdle();
        void SetWindowAdaptPass();

        const Device& device;
        MemoryAllocator& memory_allocator;
        PresentManager& present_manager;
        scheduler::Scheduler& scheduler;
        std::size_t image_count{};
        std::size_t image_index{};
        vk::Format swapchain_view_format{};

        settings::enums::ScalingFilter scaling_filter{};
        std::unique_ptr<present::WindowAdaptPass> window_adapt;
        std::list<Layer> layers;
};

}  // namespace render::vulkan