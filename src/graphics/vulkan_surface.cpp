#include "vulkan_surface.hpp"

#include "glfw_common.hpp"
#include "vulkan_common.hpp"
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
namespace vulkan {
auto createSurface(vk::Instance instance, const core::frontend::BaseWindow::WindowSystemInfo& wsi,
                   GLFWwindow* window) -> vk::SurfaceKHR {
    VkSurfaceKHR unsafe_surface = nullptr;

#ifdef _WIN32
    if (window_info.type == Core::Frontend::WindowSystemType::Windows) {
        const HWND hWnd = static_cast<HWND>(window_info.render_surface);
        const VkWin32SurfaceCreateInfoKHR win32_ci{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0, nullptr,
                                                   hWnd};
        const auto vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
            dld.vkGetInstanceProcAddr(*instance, "vkCreateWin32SurfaceKHR"));
        if (!vkCreateWin32SurfaceKHR ||
            vkCreateWin32SurfaceKHR(*instance, &win32_ci, nullptr, &unsafe_surface) != VK_SUCCESS) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Win32 surface");
            throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
        }
    }
#elif defined(__APPLE__)
    if (wsi.type == core::frontend::WindowSystemType::Cocoa) {
        const VkMetalSurfaceCreateInfoEXT macos_ci = {
            .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
            .pNext = nullptr,
            .pLayer = static_cast<const CAMetalLayer*>(::glfwGetCocoaWindow(window)),
        };
        auto result = vkCreateMetalSurfaceEXT(instance, &macos_ci, nullptr, &unsafe_surface);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Metal surface");
        }
    }
#elif defined(__ANDROID__)
    if (window_info.type == Core::Frontend::WindowSystemType::Android) {
        const VkAndroidSurfaceCreateInfoKHR android_ci{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, nullptr, 0,
                                                       reinterpret_cast<ANativeWindow*>(window_info.render_surface)};
        const auto vkCreateAndroidSurfaceKHR = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
            dld.vkGetInstanceProcAddr(*instance, "vkCreateAndroidSurfaceKHR"));
        if (!vkCreateAndroidSurfaceKHR ||
            vkCreateAndroidSurfaceKHR(*instance, &android_ci, nullptr, &unsafe_surface) != VK_SUCCESS) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Android surface");
            throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
        }
    }
#else
    if (window_info.type == Core::Frontend::WindowSystemType::X11) {
        const VkXlibSurfaceCreateInfoKHR xlib_ci{VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR, nullptr, 0,
                                                 static_cast<Display*>(window_info.display_connection),
                                                 reinterpret_cast<Window>(window_info.render_surface)};
        const auto vkCreateXlibSurfaceKHR = reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(
            dld.vkGetInstanceProcAddr(*instance, "vkCreateXlibSurfaceKHR"));
        if (!vkCreateXlibSurfaceKHR ||
            vkCreateXlibSurfaceKHR(*instance, &xlib_ci, nullptr, &unsafe_surface) != VK_SUCCESS) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Xlib surface");
            throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
        }
    }
    if (window_info.type == Core::Frontend::WindowSystemType::Wayland) {
        const VkWaylandSurfaceCreateInfoKHR wayland_ci{VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR, nullptr, 0,
                                                       static_cast<wl_display*>(window_info.display_connection),
                                                       static_cast<wl_surface*>(window_info.render_surface)};
        const auto vkCreateWaylandSurfaceKHR = reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(
            dld.vkGetInstanceProcAddr(*instance, "vkCreateWaylandSurfaceKHR"));
        if (!vkCreateWaylandSurfaceKHR ||
            vkCreateWaylandSurfaceKHR(*instance, &wayland_ci, nullptr, &unsafe_surface) != VK_SUCCESS) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Wayland surface");
            throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
        }
    }
#endif
    return unsafe_surface;
}
}  // namespace vulkan