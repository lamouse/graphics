
#include "g_context.hpp"
#include "g_window.hpp"
#include <iostream>
#include <vector>
namespace g{

Window::Window(int width, int height, ::std::string title): width{width}, height{height},title{title}
{
    initWindow();
}
void Window::initWindow()
{
    ::glfwInit();
    ::glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    ::glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = ::glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    bool enableValidationLayers = true;
    uint32_t count;
    const char** glfwExtens = ::glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extends(count);
    for(int i = 0; i < count; i++){
        extends[i] = glfwExtens[i];
    }
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    extends.push_back("VK_KHR_portability_enumeration");
#endif
    extends.push_back("VK_KHR_portability_enumeration");
    extends.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    if(enableValidationLayers)
    {
        extends.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    Context::init(extends, [&](vk::Instance instance){
        VkSurfaceKHR surface;
        auto result = ::glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if(result != VK_SUCCESS){
            throw ::std::runtime_error("createWindowSurface Fail ");
        }
        return surface;
    }, w, h, enableValidationLayers);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height){
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        Context::setExtent(w, h);
        Context::setWindowRsize();
    });
}

Window::~Window()
{   
    Context::quit();
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::shuldClose()
{
    return glfwWindowShouldClose(window);
}

}