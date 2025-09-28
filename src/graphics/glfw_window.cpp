
#include "glfw_window.hpp"

#include <spdlog/spdlog.h>

#include <utility>

#include "glfw_common.hpp"
#if defined(USE_DEBUG_UI)
#include "imgui_impl_glfw.h"
#endif

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

auto ScreenWindow::getSurface(VkInstance instance) -> VkSurfaceKHR {
    VkSurfaceKHR surface{};
    const auto result = ::glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (result != VK_SUCCESS) {
        throw ::std::runtime_error("createWindowSurface Fail ");
    }
    return surface;
}

ScreenWindow::~ScreenWindow() {
    if (window) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

void ScreenWindow::configGUI() {
#if defined(USE_DEBUG_UI)
    ImGui_ImplGlfw_InitForVulkan(window, true);
#endif
}
void ScreenWindow::destroyGUI() {
#if defined(USE_DEBUG_UI)
    ImGui_ImplGlfw_Shutdown();
#endif
}

void ScreenWindow::newFrame() {
#if defined(USE_DEBUG_UI)
    ImGui_ImplGlfw_NewFrame();
#endif
}

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
