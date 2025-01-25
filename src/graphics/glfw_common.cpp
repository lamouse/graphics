#include "glfw_common.hpp"
#include "imgui_impl_glfw.h"

#if !defined(_WIN32) && !defined(__APPLE__)
#include <qpa/qplatformnativeinterface.h>
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <QuartzCore/CAMetalLayer.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <objc/runtime.h>  // 添加这个头文件
#elif defined(_WIN32)
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
            // 获取 NSWindow 对象
            id cocoaWindow = glfwGetCocoaWindow(window);
            if (!cocoaWindow) {
                return nullptr;
            }

            // 获取 contentView
            id contentView = reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(
                cocoaWindow, sel_getUid("contentView"));
            if (!contentView) {
                return nullptr;
            }

            // 获取 layer
            id layer = reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(contentView,
                                                                       sel_registerName("layer"));
            using ObjcMsgSendFunc = BOOL (*)(id, SEL, Class);
            auto objcMsgSendFunc = reinterpret_cast<ObjcMsgSendFunc>(objc_msgSend);
            bool isCAMetalLayer =
                objcMsgSendFunc(layer, sel_registerName("isKindOfClass:"),
                                static_cast<Class>(objc_getClass("CAMetalLayer")));
            if (!isCAMetalLayer) {
                // 创建 CAMetalLayer
                id metalLayer = reinterpret_cast<id (*)(Class, SEL)>(objc_msgSend)(
                    objc_getClass("CAMetalLayer"), sel_getUid("alloc"));
                metalLayer =
                    reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(metalLayer, sel_getUid("init"));

                // 设置 contentView 的 layer
                reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(
                    contentView, sel_getUid("setLayer:"), metalLayer);
                reinterpret_cast<void (*)(id, SEL, BOOL)>(objc_msgSend)(
                    contentView, sel_getUid("setWantsLayer:"), YES);

                layer = metalLayer;
            }

            // 检测并设置 contentsScale
            // if (glfwGetWindowAttrib(window, GLFW_SCALE_TO_MONITOR)) {
            reinterpret_cast<void (*)(id, SEL, CGFloat)>(objc_msgSend)(
                layer, sel_getUid("setContentsScale:"),
                reinterpret_cast<CGFloat (*)(id, SEL)>(objc_msgSend)(
                    cocoaWindow, sel_getUid("backingScaleFactor")));
            //}

            return layer;
        }
#elif defined(GLFW_EXPOSE_NATIVE_WIN32)
        case sys_type_enum::Windows:
            return glfwGetWin32Window(window);
#endif
        default:
            return nullptr;
    }
}

void init_glfw_imgui(GLFWwindow* window) { ImGui_ImplGlfw_InitForVulkan(window, true); }
}  // namespace GLFWCommon