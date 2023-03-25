#include "g_context.hpp"
#include <iostream>
#include<vector>
#include<cassert>
namespace g{


::std::unique_ptr<Context> Context::pInstance = nullptr;
int Context::width = 800;
int Context::height = 600;

Context::Context(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc)
{
    Device::init(instanceExtends, createFunc);

#if defined(VK_USE_PLATFORM_MACOS_MVK)
    ::std::string full_path{"/Users/sora/project/cpp/test/xmake/graphics/src/shader/"};
#else
    ::std::string full_path{"E:/project/cpp/graphics/src/shader/"};
#endif
    
    Shader::init(full_path + "vert.spv", 
            full_path + "frag.spv");
    Swapchain::init(width, height);
    RenderProcess::init(width, height);
    Command::init(Swapchain::getInstance().getImageCount());
    
}

Context::~Context()
{
    Device::getInstance().getVKDevice().waitIdle();
    Command::quit();
    RenderProcess::quit();
    Swapchain::quit();
    Shader::quit();
    Device::quit();
}

void Context::init(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc, int width, int height)
{
    width = width;
    height = height;
    pInstance.reset(new Context(instanceExtends, createFunc));
}
void Context::quit()
{
    pInstance.reset();
}

Context& Context::Instance()
{   
    assert(pInstance);
    return *pInstance;
}

}

