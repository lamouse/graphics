#include "g_context.hpp"
#include "g_pipeline.hpp"
#include "g_swapchain.hpp"
#include <iostream>
#include<vector>
#include<cassert>
namespace g{


::std::unique_ptr<Context> Context::pInstance = nullptr;
int Context::width = 800;
int Context::height = 600;
bool Context::windowIsRsize = false;

Context::Context(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc)
{
    Device::init(instanceExtends, createFunc);
}

Context::~Context()
{
    Device::getInstance().getVKDevice().waitIdle();
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

