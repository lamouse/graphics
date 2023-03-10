#ifndef G_CONTEXT_HPP
#define G_CONTEXT_HPP
#include "g_device.hpp"
#include "g_swapchain.hpp"
#include "g_shader.hpp"
#include "g_render.hpp"
#include "g_command.hpp"
#include<memory>
#include <functional>

namespace g{
    
    class Context final
    {
    private:
        Context(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc);
        static ::std::unique_ptr<Context> pInstance;
        ::vk::Instance createInstance(const std::vector<const char*>& instanceExtends);
        ::vk::SwapchainKHR swapchain;
        ::vk::Device device;
        static int width;
        static int height;
    public:
        static void init(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc, int width, int height);
        static void quit();
        static Context& Instance();
        ~Context();
    };
    
    
}

#endif