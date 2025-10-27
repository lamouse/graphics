#include "render_vulkan.hpp"
#include "common/scope_exit.h"
#include "vulkan_common/instance.hpp"
#include "common/settings.hpp"
#include <spdlog/spdlog.h>
#include "vulkan_common/debug_callback.hpp"
#include "vulkan_common/vk_surface.hpp"
#include "present/vulkan_utils.hpp"

namespace render::vulkan {
auto createDevice(const Instance& instance, vk::SurfaceKHR surface) -> Device {
    auto physical = instance.EnumeratePhysicalDevices();
    if (physical.size() == 1) {
        return Device(*instance, physical[0], surface);
    }
    for (auto device : physical) {
        vk::PhysicalDeviceProperties properties =  device.getProperties();
        switch (properties.deviceType) {
            case vk::PhysicalDeviceType::eDiscreteGpu:
                SPDLOG_INFO("设备类型：离散 GPU（独立显卡）, 设备名称：{}", std::string(properties.deviceName.data()));
                return Device(*instance, device, surface);
            case vk::PhysicalDeviceType::eIntegratedGpu:
                SPDLOG_INFO("设备类型：集成 GPU（如 Intel UHD）设备名称：{}", std::string(properties.deviceName.data()));
                break;
            case vk::PhysicalDeviceType::eVirtualGpu:
                SPDLOG_INFO("设备类型：虚拟 GPU（如远程渲染）设备名称：{}", std::string(properties.deviceName.data()));
                break;
            case vk::PhysicalDeviceType::eCpu:
                SPDLOG_INFO("设备类型：CPU（软件模拟）设备名称：{}", std::string(properties.deviceName.data()));
                break;
            default:
                SPDLOG_INFO("设备类型：未知");
                break;
        }
    }
}

RendererVulkan::RendererVulkan(core::frontend::BaseWindow* window) try
    : RenderBase(window),
      instance(createInstance(VK_API_VERSION_1_3, window->getWindowSystemInfo().type,
                              common::settings::get<settings::RenderVulkan>().render_debug)),
      debug_messenger(common::settings::get<settings::RenderVulkan>().render_debug
                          ? createDebugMessenger(*instance)
                          : DebugUtilsMessenger{}),
      surface(createSurface(*instance, window->getWindowSystemInfo())),
      device(createDevice(instance, *surface)),
      memory_allocator(device),
      scheduler(device),
      swapchain(*surface, device, scheduler, window->getFramebufferLayout().width,
                window->getFramebufferLayout().height),
      present_manager(*instance, *window, device, memory_allocator, scheduler, swapchain, surface),
      blit_swapchain(device, memory_allocator, present_manager, scheduler),
      blit_capture(device, memory_allocator, present_manager, scheduler),
      imgui(std::make_unique<ImguiCore>(window, device, device.getPhysical(), *instance,
                                        window->getWindowSystemInfo().render_surface_scale)),
      vulkan_graphics(window, device, memory_allocator, scheduler, getShaderNotify(), imgui.get()) {
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
    SCOPE_EXIT->void { window_->OnFrameDisplayed(); };
    if (!window_->IsShown()) {
        return;
    }
    RenderScreenshot(frame_buffers);
    Frame* frame = present_manager.getRenderFrame();
    imgui->newFrame();
    blit_swapchain.DrawToFrame(vulkan_graphics, frame, window_->getFramebufferLayout(),
                               frame_buffers, swapchain.getImageCount(),
                               swapchain.getImageViewFormat());
    if (imgui_ui) {
        imgui_ui();
    } else {
        imgui->imgui_predraw();
    }
    {
        std::scoped_lock submit_lock{scheduler.submit_mutex_};
        imgui->endFrame();
    }
    scheduler.flush(*frame->render_ready);
    present_manager.present(frame);
    vulkan_graphics.TickFrame();
    imgui_ui = nullptr;
}

void RendererVulkan::RenderScreenshot(std::span<const frame::FramebufferConfig> framebuffers) {
    // TODO 没有实现
}

auto RendererVulkan::RenderToBuffer(std::span<const frame::FramebufferConfig> framebuffers,
                                    const layout::FrameBufferLayout& layout, vk::Format format,
                                    vk::DeviceSize buffer_size) -> Buffer {
    auto frame = [&]() {
        Frame f{};
        f.image = present::utils::CreateWrappedImage(
            memory_allocator, vk::Extent2D{layout.width, layout.height}, format);
        f.image_view = present::utils::CreateWrappedImageView(device, f.image, format);
        f.framebuffer = blit_capture.CreateFramebuffer(*f.image_view, layout, format);
        return f;
    }();

    auto dst_buffer =
        present::utils::CreateWrappedBuffer(memory_allocator, buffer_size, MemoryUsage::Download);
    blit_capture.DrawToFrame(vulkan_graphics, &frame, layout, framebuffers, 1, format);

    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([&](vk::CommandBuffer cmdbuf) {
        present::utils::DownloadColorImage(cmdbuf, *frame.image, *dst_buffer,
                                           vk::Extent3D{layout.width, layout.height, 1});
    });

    // Ensure the copy is fully completed before saving the capture
    scheduler.finish();

    // Copy backing image data to the capture buffer
    dst_buffer.Invalidate();
    return dst_buffer;
}

auto RendererVulkan::getAppletCaptureBuffer() -> std::vector<u8> {
    std::vector<u8> out(1920 * 1080);

    if (!applet_frame.image) {
        return out;
    }

    const auto dst_buffer =
        present::utils::CreateWrappedBuffer(memory_allocator, 1920 * 1080, MemoryUsage::Download);

    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([&](vk::CommandBuffer cmdbuf) {
        present::utils::DownloadColorImage(cmdbuf, *applet_frame.image, *dst_buffer,
                                           {1920, 1080, 1});
    });

    // Ensure the copy is fully completed before writing the capture
    scheduler.finish();

    // Swizzle image data to the capture buffer
    dst_buffer.Invalidate();
    // TODO 未完成

    return out;
}

void RendererVulkan::RenderAppletCaptureLayer(
    std::span<const frame::FramebufferConfig> framebuffers) {
    // if (!applet_frame.image) {
    //     applet_frame.image = CreateWrappedImage(memory_allocator, CaptureImageSize,
    //     CaptureFormat); applet_frame.image_view = CreateWrappedImageView(device,
    //     applet_frame.image, CaptureFormat); applet_frame.framebuffer =
    //     blit_applet.CreateFramebuffer(
    //         VideoCore::Capture::Layout, *applet_frame.image_view, CaptureFormat);
    // }

    // blit_applet.DrawToFrame(rasterizer, &applet_frame, framebuffers,
    // VideoCore::Capture::Layout,
    // 1,
    //                         CaptureFormat);
}

}  // namespace render::vulkan