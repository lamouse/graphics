
#include "glfw_window.hpp"

#include <spdlog/spdlog.h>

#include <utility>

#include "glfw_common.hpp"
#include "imgui_impl_glfw.h"
#include <GLFW/glfw3native.h>

namespace graphics {

ScreenWindow::ScreenWindow(int width, int height, ::std::string title) : title_{std::move(title)} {
    ::glfwInit();
    ::glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    ::glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    //::glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER,GLFW_TRUE);

#if defined(GLFW_EXPOSE_NATIVE_COCOA)
    window_info.render_surface_scale = 1.0f;
#else
    float x_scale{}, y_scale{};
    ::GLFWmonitor* monitor = ::glfwGetPrimaryMonitor();
    ::glfwGetMonitorContentScale(monitor, &x_scale, &y_scale);
    window_info.render_surface_scale = x_scale;
#endif
    core::frontend::BaseWindow::WindowConfig conf;
    conf.extent.width = width;
    conf.extent.height = height;
    conf.fullscreen = false;
    setWindowConfig(conf);
    window =
        ::glfwCreateWindow(conf.extent.width, conf.extent.height, title_.c_str(), nullptr, nullptr);
    if (!window) {
        ::glfwTerminate();
    }
    window_info.type = GLFWCommon::get_window_system_info();
    window_info.render_surface = GLFWCommon::get_windows_handles(window);
#if defined(GLFW_EXPOSE_NATIVE_WAYLAND)
    if (window_info.type == core::frontend::WindowSystemType::Wayland) {
        // window_info.display_connection = glfwGetWaylandDisplay();
    }
#endif
#if defined(GLFW_EXPOSE_NATIVE_X11)
    if (window_info.type == core::frontend::WindowSystemType::X11) {
        window_info.display_connection = glfwGetX11Display();
    }
#endif

    initWindow();
}
void ScreenWindow::initWindow() {
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* glfw_window, int, int) {
        int w{}, h{};
        glfwGetFramebufferSize(glfw_window, &w, &h);
        auto* base_window = reinterpret_cast<ScreenWindow*>(glfwGetWindowUserPointer(glfw_window));
        if (h == 0 || w == 0) {
            return;
        }
        base_window->UpdateCurrentFramebufferLayout(static_cast<uint32_t>(w),
                                                    static_cast<uint32_t>(h));
    });
    // 设置窗口最小尺寸为 160x90
    glfwSetWindowSizeLimits(window, 160, 90, GLFW_DONT_CARE, GLFW_DONT_CARE);
}

ScreenWindow::~ScreenWindow() {
    if (window) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

void ScreenWindow::configGUI() { ImGui_ImplGlfw_InitForVulkan(window, true); }
void ScreenWindow::destroyGUI() { ImGui_ImplGlfw_Shutdown(); }

void ScreenWindow::newFrame() { ImGui_ImplGlfw_NewFrame(); }

ScreenWindow::ScreenWindow(ScreenWindow&& w) noexcept
    : window(w.window), title_(std::move(w.title_)) {
    spdlog::info("Window move constructor");
    w.window = nullptr;
    w.title_ = "";
    setWindowConfig(w.getWindowConfig());
    window_info = w.window_info;
}
auto ScreenWindow::operator=(ScreenWindow&& w) noexcept -> ScreenWindow& {
    spdlog::info("Window move assignment");
    window = w.window;
    w.window = nullptr;
    setWindowConfig(w.getWindowConfig());
    window_info = w.window_info;
    title_ = std::move(w.title_);
    w.title_ = "";
    return *this;
}

[[nodiscard]] auto ScreenWindow::IsShown() const -> bool {
    return glfwGetWindowAttrib(window, GLFW_VISIBLE) != 0;
}
[[nodiscard]] auto ScreenWindow::IsMinimized() const -> bool {
    return glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0;
}

auto ScreenWindow::shouldClose() const -> bool { return glfwWindowShouldClose(window); }

}  // namespace graphics
