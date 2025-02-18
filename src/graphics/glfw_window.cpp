
#include "glfw_window.hpp"

#include <spdlog/spdlog.h>

#include <utility>
#include <vector>

#include "glfw_common.hpp"
#include "imgui_impl_glfw.h"

namespace g {

ScreenWindow::ScreenWindow(ScreenExtent extent, ::std::string title) : title_{std::move(title)} {
    ::glfwInit();
    ::glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    ::glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    //::glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER,GLFW_TRUE);

#if defined(VK_USE_PLATFORM_MACOS_MVK)
    window_info.render_surface_scale = 1.0f;
#else
    float x_scale{}, y_scale{};
    ::GLFWmonitor* monitor = ::glfwGetPrimaryMonitor();
    ::glfwGetMonitorContentScale(monitor, &x_scale, &y_scale);
    window_info.render_surface_scale = x_scale;
#endif
    core::frontend::BaseWindow::WindowConfig conf;
    conf.extent.width = extent.width;
    conf.extent.height = extent.height;
    conf.fullscreen = false;
    setWindowConfig(conf);
    window = ::glfwCreateWindow(conf.extent.width,  conf.extent.height, title_.c_str(), nullptr, nullptr);
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
        base_window->UpdateCurrentFramebufferLayout(static_cast<uint32_t>(w),
                                                    static_cast<uint32_t>(h));
    });
}

auto ScreenWindow::getSurface(VkInstance instance) -> VkSurfaceKHR {
    VkSurfaceKHR surface{};
    const auto result = ::glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (result != VK_SUCCESS) {
        throw ::std::runtime_error("createWindowSurface Fail ");
    }
    return surface;
}

auto ScreenWindow::getRequiredInstanceExtends(bool enableValidationLayers)
    -> ::std::vector<const char*> {
    uint32_t count{0};
    const char** glfwExtends = ::glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extends(count);
    for (uint32_t i = 0; i < count; i++) {
        extends[i] = glfwExtends[i];
    }
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    extends.push_back("VK_KHR_portability_enumeration");
    extends.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif
    if (enableValidationLayers) {
        extends.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extends;
}

auto ScreenWindow::getExtent() -> ScreenExtent {
    int width{}, height{};
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
    }
    return {width, height};
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

}  // namespace g
