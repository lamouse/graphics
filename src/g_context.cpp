#include "g_context.hpp"
#include <iostream>
#include <utility>
#include <vector>
#include <cassert>
namespace g{
::std::unique_ptr<core::Device> device = nullptr;

::std::unique_ptr<Context> Context::pInstance = nullptr;
int Context::width_ = 0;
int Context::height_ = 0;
bool Context::windowIsResize = false;

Context::Context(const std::vector<const char*>& instanceExtends, core::CreateSurfaceFunc createFunc, bool enableValidationLayers)
{
   device_ = ::std::make_shared<core::Device>(instanceExtends, createFunc, enableValidationLayers);
   imageQualityConfig.msaaSamples = device_->getMaxMsaaSamples();
}

Context::~Context()
{
    device_->logicalDevice().waitIdle();
}

void Context::init(std::vector<const char*>& instanceExtends, core::CreateSurfaceFunc createFunc, int width, int height, bool enableValidationLayers)
{
    width_ = width;
    height_ = height;
    pInstance.reset(new Context(instanceExtends, std::move(createFunc), enableValidationLayers));
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

