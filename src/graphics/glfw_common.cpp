#include "glfw_common.hpp"

#if !defined(WIN32) && !defined(__APPLE__)
#include <qpa/qplatformnativeinterface.h>
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <objc/message.h>
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64)
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

namespace GLFWCommon {
auto get_window_system_info() -> core::frontend::WindowSystemType {
    auto platform = glfwGetPlatform();
    switch (platform) {
        case GLFW_PLATFORM_WIN32:
            return core::frontend::WindowSystemType::Windows;
        case GLFW_PLATFORM_COCOA:
            return core::frontend::WindowSystemType::Cocoa;
        case GLFW_PLATFORM_X11:
            return core::frontend::WindowSystemType::X11;
        case GLFW_PLATFORM_WAYLAND:
            return core::frontend::WindowSystemType::Wayland;
        default:
            return core::frontend::WindowSystemType::Headless;
    }
}
auto get_windows_handles(GLFWwindow* window) -> void* {
    using sys_type_enum = core::frontend::WindowSystemType;
    auto sys_type = get_window_system_info();
    switch (sys_type) {
#if defined(GLFW_EXPOSE_NATIVE_COCOA)
        case sys_type_enum::Cocoa: {
            auto* id = ::glfwGetCocoaWindow(window);
            return id;
        }
#elif defined(GLFW_EXPOSE_NATIVE_WIN32)
#endif
        default:
            return nullptr;
    }
}
}  // namespace GLFWCommon