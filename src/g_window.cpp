
#include "g_window.hpp"

#include <spdlog/spdlog.h>

#include <utility>
#include <vector>

#include "g_context.hpp"

namespace g {

Window::Window(ScreenExtent extent, ::std::string title)
    : width{extent.width}, height{extent.height}, title_{std::move(title)} {
    Context::setExtent({width, height});
    Context::resetWindowResize();
    ::glfwInit();
    ::glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    ::glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    //::glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER,GLFW_TRUE);
    ::GLFWmonitor* monitor = ::glfwGetPrimaryMonitor();
    float xscale{}, yscale{};
    ::glfwGetMonitorContentScale(monitor, &xscale, &yscale);
    int w = static_cast<int>((float)width * xscale);
    int h = static_cast<int>((float)height * yscale);
    scale = xscale;
    window = ::glfwCreateWindow(w, h, title_.c_str(), nullptr, nullptr);
    if (!window) {
        ::glfwTerminate();
    }
    initWindow();
}
void Window::initWindow() {
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int, int) {
        int w{}, h{};
        glfwGetFramebufferSize(window, &w, &h);
        Context::setExtent({w, h});
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
    const char** glfwExtens = ::glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extends(count);
    for (uint32_t i = 0; i < count; i++) {
        extends[i] = glfwExtens[i];
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

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

auto Window::shouldClose() -> bool { return glfwWindowShouldClose(window); }

}  // namespace g