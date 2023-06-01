
#include "g_context.hpp"
#include "g_window.hpp"
#include <iostream>
#include <utility>
#include <vector>
namespace g{

Window::Window(int width, int height, ::std::string title): width{width}, height{height},title{std::move(title)}
{
    initWindow();
}
void Window::initWindow()
{
    ::glfwInit();
    ::glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    ::glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    //::glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER,GLFW_TRUE);
    ::GLFWmonitor* monitor = ::glfwGetPrimaryMonitor();
    float xscale, yscale;
    ::glfwGetMonitorContentScale(monitor, &xscale, &yscale);
    int w = width * xscale;
    int h = height * yscale;
    scale = xscale;
    window = ::glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
    if(!window)
    {
        ::glfwTerminate();
    }
    bool enableValidationLayers = true;
    uint32_t count;
    const char** glfwExtens = ::glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extends(count);
    for(int i = 0; i < count; i++){
        extends[i] = glfwExtens[i];
    }
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    extends.push_back("VK_KHR_portability_enumeration");
    extends.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif
    if(enableValidationLayers)
    {
        extends.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    Context::init(extends, [&](vk::Instance instance){
        VkSurfaceKHR surface;
        auto result = ::glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if(result != VK_SUCCESS){
            throw ::std::runtime_error("createWindowSurface Fail ");
        }
        return surface;
    }, width, height, enableValidationLayers);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow * /*window*/, int width, int height){
        Context::setExtent(width, height);
        Context::setWindowResize();
    });
}

Window::~Window()
{   
    Context::quit();
    glfwDestroyWindow(window);
    glfwTerminate();
}

auto Window::shuldClose() -> bool
{
    return glfwWindowShouldClose(window);
}

}