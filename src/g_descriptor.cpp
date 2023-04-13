#include "g_descriptor.hpp"
#include "g_defines.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/spdlog.h>

#include <iostream>
namespace g {


void Descriptor::createDescriptorPool(::vk::Device& device, uint32_t poolSize)
{
    std::array<::vk::DescriptorPoolSize, 11> poolSizes{
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eCombinedImageSampler, poolSize),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eSampler, poolSize),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eSampledImage, poolSize),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageImage, poolSize),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eUniformTexelBuffer, poolSize),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageTexelBuffer, poolSize),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eUniformBuffer, poolSize),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageBuffer, poolSize),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eUniformBufferDynamic, poolSize),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageBufferDynamic, poolSize),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eInputAttachment, poolSize)
    };
    
    ::vk::DescriptorPoolCreateInfo createInfo;
    createInfo.setPoolSizes(poolSizes)
               .setMaxSets(poolSize);
    descriptorPool_ =  device.createDescriptorPool(createInfo); 
    is_init = true;
    spdlog::debug(DETAIL_INFO("init createDescriptorPool"));
}

void Descriptor::destoryDescriptorPool(::vk::Device& device)
{
    if(!is_init)
    {

    }else {
        device.destroyDescriptorPool(descriptorPool_);
        is_destroy = true;
    }

}

Descriptor::~Descriptor()
{
    if(!is_destroy && is_init)
    {
        spdlog::error("un destroyDescriptorPool");
    }
}
}