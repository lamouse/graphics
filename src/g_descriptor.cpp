#include "g_descriptor.hpp"

namespace g {

Descriptor::Descriptor(::vk::Device device, uint32_t poolSize):device_(device)
{
    createDescriptorPool(poolSize);
}

void Descriptor::createDescriptorPool(uint32_t count)
{
    std::array<::vk::DescriptorPoolSize, 11> poolSizes{
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eCombinedImageSampler, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eSampler, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eSampledImage, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageImage, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eUniformTexelBuffer, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageTexelBuffer, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eUniformBuffer, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageBuffer, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eUniformBufferDynamic, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eStorageBufferDynamic, count),
        ::vk::DescriptorPoolSize(::vk::DescriptorType::eInputAttachment, count)
    };
    
    ::vk::DescriptorPoolCreateInfo createInfo;
    createInfo.setPoolSizes(poolSizes)
               .setMaxSets(count);
    descriptorPool_ =  device_.get().createDescriptorPool(createInfo); 
}

Descriptor::~Descriptor()
{
    device_.get().destroyDescriptorPool(descriptorPool_);
}
}