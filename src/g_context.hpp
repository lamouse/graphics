#ifndef G_CONTEXT_HPP
#define G_CONTEXT_HPP
#include "g_device.hpp"

#include<memory>
#include <functional>
#include <GLFW/glfw3.h>

namespace g{
    
    class Context final
    {
    private:
        Context(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc);
        static ::std::unique_ptr<Context> pInstance;
        static int width;
        static int height;
        static bool windowIsRsize;
    public:
        static void setExtent(int width, int height){width = width; height = height;}
        static void setWindowRsize(){windowIsRsize = true;}
        static void rsetWindowRsize(){windowIsRsize = false;}
        static bool isWindowRsize(){ return windowIsRsize;}
        static ::vk::Extent2D getExtent(){return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};}
        static void init(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc, int width, int height);
        static void quit();
        static void waitWindowEvents(){glfwWaitEvents();}
        static Context& Instance();
        ~Context();
    };
    
    
}

#endif