#include "g_context.hpp"
#include "g_pipeline.hpp"
#include "g_swapchain.hpp"
#include <iostream>
#include <vector>
#include <cassert>
namespace g{
::std::unique_ptr<core::Device> device = nullptr;

::std::unique_ptr<Context> Context::pInstance = nullptr;
int Context::width = 800;
int Context::height = 600;
bool Context::windowIsRsize = false;

Context::Context(const std::vector<const char*>& instanceExtends, core::CreateSurfaceFunc createFunc)
{
   device_ = ::std::make_shared<core::Device>(instanceExtends, createFunc);
   imageQualityConfig.msaaSamples = device_->getMaxMsaaSamples();
}

Context::~Context()
{
    device_->logicalDevice().waitIdle();
}

void Context::init(const std::vector<const char*>& instanceExtends, core::CreateSurfaceFunc createFunc, int width, int height)
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

