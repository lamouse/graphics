module;
#include "common/scope_exit.h"
#include <spdlog/spdlog.h>

#include <tracy/Tracy.hpp>
#include <vulkan/vulkan.hpp>
module render.impl.vulkan;
import render.vulkan.common;
import render.vulkan.present.present_frame;
import render;
import core;
import common;

namespace render::vulkan {
auto createDevice(const Instance& instance, vk::SurfaceKHR surface) -> Device {
    auto physical = instance.EnumeratePhysicalDevices();

    for (auto device : physical) {
        vk::PhysicalDeviceProperties properties = device.getProperties();
        switch (properties.deviceType) {
            case vk::PhysicalDeviceType::eDiscreteGpu:
                SPDLOG_INFO("设备类型：离散 GPU（独立显卡）, 设备名称：{}",
                            std::string(properties.deviceName.data()));
                return Device(*instance, device, surface);
            case vk::PhysicalDeviceType::eIntegratedGpu:
                SPDLOG_INFO("设备类型：集成 GPU（如 Intel UHD）设备名称：{}",
                            std::string(properties.deviceName.data()));
                return Device(*instance, device, surface);
            case vk::PhysicalDeviceType::eVirtualGpu:
                SPDLOG_INFO("设备类型：虚拟 GPU（如远程渲染）设备名称：{}",
                            std::string(properties.deviceName.data()));
                return Device(*instance, device, surface);
            case vk::PhysicalDeviceType::eCpu:
                SPDLOG_INFO("设备类型：CPU（软件模拟）设备名称：{}",
                            std::string(properties.deviceName.data()));
                return Device(*instance, device, surface);
            default:
                SPDLOG_INFO("设备类型：未知");
                return Device(*instance, device, surface);
                break;
        }
    }
    throw std::runtime_error("no gpu device select");
}

RendererVulkan::RendererVulkan(core::frontend::BaseWindow* window) try
    : RenderBase(window),
      instance(createInstance(VK_API_VERSION_1_3, window->getWindowSystemInfo().type,
                              settings::values.render_debug.GetValue())),
      debug_messenger(settings::values.render_debug.GetValue() ? createDebugMessenger(*instance)
                                                               : DebugUtilsMessenger{}),
      surface(createSurface(instance, window->getWindowSystemInfo())),
      device(createDevice(instance, *surface)),
      memory_allocator(device),
      scheduler(device),
      swapchain(*surface, device, window->getFramebufferLayout().width,
                window->getFramebufferLayout().height),
      present_manager(instance, *window, device, memory_allocator, scheduler, swapchain, surface),
      blit_swapchain(device, memory_allocator, present_manager, scheduler),
      imgui(settings::values.use_debug_ui.GetValue()
                ? std::make_unique<ImguiCore>(window, device, scheduler, device.getPhysical(),
                                              *instance)
                : nullptr),
      vulkan_graphics(window, device, memory_allocator, scheduler, getShaderNotify()) {
} catch (const std::exception& exception) {
    SPDLOG_ERROR("Vulkan initialization failed with error: {}", exception.what());
    throw std::runtime_error{fmt::format("Vulkan initialization error {}", exception.what())};
}

RendererVulkan::~RendererVulkan() { void(device.getLogical().waitIdle()); }

void RendererVulkan::composite(std::span<frame::FramebufferConfig> frame_buffers,
                               const imgui_ui_fun& func) {
    ZoneScopedNC("RendererVulkan::composite", 130);
    if (frame_buffers.empty()) {
        return;
    }

    SCOPE_EXIT->void { window_->OnFrameDisplayed(); };
    if (!window_->IsShown()) {
        return;
    }

    Frame* frame = present_manager.getRenderFrame();

        auto accelerateDisplay = [&](const render::frame::FramebufferConfig & framebuffer, unsigned int stride){
            return vulkan_graphics.AccelerateDisplay(framebuffer,stride);
        };
    blit_swapchain.DrawToFrame(accelerateDisplay, frame, window_->getFramebufferLayout(),
                               frame_buffers, swapchain.getImageCount(),
                               swapchain.getImageViewFormat());
    if (imgui && func) {
        imgui->draw(func, frame);
    }
    scheduler.flush(*frame->render_ready);
    present_manager.present(frame);
    vulkan_graphics.TickFrame();
}

}  // namespace render::vulkan