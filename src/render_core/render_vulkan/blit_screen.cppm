module;

#include "common/settings_enums.hpp"
#include <list>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <functional>

export module render.vulkan.blit_screen;

import render.vulkan.common;
import render.vulkan.present.present_frame;
import render.vulkan.present_manager;
import render.vulkan.present.layer;
import render.vulkan.present.window_adapt_pass;
import render.vulkan.scheduler;
import core;
import render;


export  namespace render::vulkan {
class BlitScreen {
    public:
        explicit BlitScreen(const Device& device, MemoryAllocator& memory_allocator,
                            PresentManager& present_manager, scheduler::Scheduler& scheduler);
        ~BlitScreen();

        void DrawToFrame(const std::function<std::optional<FramebufferTextureInfo>(const frame::FramebufferConfig& framebuffer,
                               uint32_t stride)>& accelerateDisplay, Frame* frame,
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
}