#include "render_vulkan.hpp"
#include "common/scope_exit.h"
#include "vulkan_common/instance.hpp"
#include "common/settings.hpp"
#include <spdlog/spdlog.h>
#include "vulkan_common/debug_callback.hpp"
#include "vulkan_common/vk_surface.hpp"

namespace render::vulkan {
auto createDevice(const Instance& instance, vk::SurfaceKHR surface) -> Device {
    auto physical = instance.EnumeratePhysicalDevices();
    return Device(*instance, physical[0], surface);
}

RendererVulkan::RendererVulkan(core::frontend::BaseWindow& window) try
    : RenderBase(&window),
      instance(createInstance(VK_API_VERSION_1_3, window.getWindowSystemInfo().type,
                              common::settings::get<settings::RenderVulkan>().render_debug)),
      debug_messenger(common::settings::get<settings::RenderVulkan>().render_debug
                          ? createDebugMessenger(*instance)
                          : DebugUtilsMessenger{}),
      surface(createSurface(*instance, window.getWindowSystemInfo())),
      device(createDevice(instance, *surface)),
      memory_allocator(device),
      scheduler(device),
      swapchain(*surface, device, scheduler, window.getFramebufferLayout().width,
                window.getFramebufferLayout().height),
      present_manager(*instance, window, device, memory_allocator, scheduler, swapchain, surface),
      blit_swapchain(device, memory_allocator, present_manager, scheduler),
      rasterizer(window, device, memory_allocator, scheduler),
      applet_frame() {
    if (common::settings::get<settings::RenderVulkan>().renderer_force_max_clock &&
        device.shouldBoostClocks()) {
        turbo_mode.emplace(instance);
        scheduler.registerOnSubmit([this] { turbo_mode->QueueSubmitted(); });
    }
} catch (const std::exception& exception) {
    SPDLOG_ERROR("Vulkan initialization failed with error: {}", exception.what());
    throw std::runtime_error{fmt::format("Vulkan initialization error {}", exception.what())};
}

RendererVulkan::~RendererVulkan() {
    scheduler.registerOnSubmit([] {});
    void(device.getLogical().waitIdle());
}

void RendererVulkan::composite(std::span<frame::FramebufferConfig> frame_buffers) {
    if (frame_buffers.empty()) {
        return;
    }

    RenderAppletCaptureLayer(frame_buffers);
    SCOPE_EXIT { window_->OnFrameDisplayed(); };
    if (!window_->IsShown()) {
        return;
    }
    RenderScreenshot(frame_buffers);
    Frame* frame = present_manager.getRenderFrame();
    scheduler.flush(*frame->render_ready);
    blit_swapchain.DrawToFrame(rasterizer, frame, frame_buffers, swapchain.getImageCount(),
                               swapchain.getImageViewFormat());
    rasterizer.TickFrame();
}

void RendererVulkan::RenderScreenshot(std::span<const frame::FramebufferConfig> framebuffers) {
    // TODO 没有实现
}

}  // namespace render::vulkan