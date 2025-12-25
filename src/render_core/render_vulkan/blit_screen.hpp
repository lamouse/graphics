#pragma once

#include "common/common_types.hpp"
#include "common/settings_enums.hpp"
#include "render_core/framebuffer_config.hpp"
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include <list>
#include <memory>
#include "core/frontend/framebuffer_layout.hpp"

namespace render::vulkan {
class Device;
class PresentManager;
class MemoryAllocator;
class Layer;
class VulkanGraphics;
class Framebuffer;
namespace present {
class WindowAdaptPass;
}

namespace scheduler {
class Scheduler;
}
struct Frame;

struct FramebufferTextureInfo {
        vk::Image image;
        vk::ImageView image_view;
        u32 width{};
        u32 height{};
        u32 scaled_width{};
        u32 scaled_height{};
};

class BlitScreen {
    public:
        explicit BlitScreen(const Device& device, MemoryAllocator& memory_allocator,
                            PresentManager& present_manager, scheduler::Scheduler& scheduler);
        ~BlitScreen();

        void DrawToFrame(VulkanGraphics& rasterizer, Frame* frame,
                         const layout::FrameBufferLayout& layout,
                         std::span<const frame::FramebufferConfig> framebuffers,
                         size_t current_swapchain_image_count,
                         vk::Format current_swapchain_view_format);

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