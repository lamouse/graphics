
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

    uint32_t count;
    const char** glfwExtens = ::glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extends(count);
    for(int i = 0; i < count; i++){
        extends[i] = glfwExtens[i];
    }
    Context::init(extends, [&](vk::Instance instance){
        VkSurfaceKHR surface;
        auto result = ::glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if(result != VK_SUCCESS){
            throw ::std::runtime_error("createWindowSurface Fail ");
        }
        return surface;
    }, width, height);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height){
        Context::setExtent(width, height);
        Context::setWindowRsize();
    });
}

Window::~Window()
{   
    Context::quit();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::draw(::std::string resourcePath)
{

}

bool Window::shuldClose()
{
    return glfwWindowShouldClose(window);
}

}