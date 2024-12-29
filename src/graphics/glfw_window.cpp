
#include "glfw_window.hpp"

#include <spdlog/spdlog.h>

#include <utility>
#include <vector>

#include "glfw_common.hpp"

namespace g {

Window::Window(ScreenExtent extent, ::std::string title) : title_{std::move(title)} {
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
    WindowConfig config;
    config.extent.width = extent.width;
    config.extent.height = extent.height;
    config.fullscreen = false;
    setWindowConfig(config);
    int const w = static_cast<int>((float)config.extent.width * window_info.render_surface_scale);
    int const h = static_cast<int>((float)config.extent.height * window_info.render_surface_scale);
    window = ::glfwCreateWindow(w, h, title_.c_str(), nullptr, nullptr);
    if (!window) {
        ::glfwTerminate();
    }
    window_info.type = GLFWCommon::get_window_system_info();
    window_info.get_surface = GLFWCommon::get_windows_handles(window);
    initWindow();
}
void Window::initWindow() {
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* glfw_window, int, int) {
        int w{}, h{};
        glfwGetFramebufferSize(glfw_window, &w, &h);
    });
}

auto Window::getSurface(VkInstance instance) -> VkSurfaceKHR {
    VkSurfaceKHR surface{};
    const auto result = ::glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (result != VK_SUCCESS) {
        throw ::std::runtime_error("createWindowSurface Fail ");
    }
    return surface;
}

auto Window::getRequiredInstanceExtends(bool enableValidationLayers) -> ::std::vector<const char*> {
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

auto Window::getExtent() -> ScreenExtent {
    int width{}, height{};
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
    }
    return {width, height};
}

Window::~Window() {
    if (window) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

Window::Window(Window&& w) noexcept : window(w.window), title_(std::move(w.title_)) {
    spdlog::info("Window move constructor");
    w.window = nullptr;
    w.title_ = "";
    setWindowConfig(w.getWindowConfig());
    window_info = w.window_info;
}
auto Window::operator=(Window&& w) noexcept -> Window& {
    spdlog::info("Window move assignment");
    window = w.window;
    w.window = nullptr;
    setWindowConfig(w.getWindowConfig());
    window_info = w.window_info;
    title_ = std::move(w.title_);
    w.title_ = "";
    return *this;
}

[[nodiscard]] auto Window::IsShown() const -> bool { return glfwGetWindowAttrib(window, GLFW_VISIBLE) != 0; }
[[nodiscard]] auto Window::IsMinimized() const -> bool { return glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0; }

auto Window::shouldClose() const -> bool { return glfwWindowShouldClose(window); }

}  // namespace g
