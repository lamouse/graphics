#include "g_descriptor.hpp"
#include "g_defines.hpp"

#include <iostream>
#include <cassert>
namespace g {


DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::addBinding(
    uint32_t binding, ::vk::DescriptorType type,
    ::vk::ShaderStageFlags stageFlags, uint32_t count) {
    assert(descriptorSetLayoutBindings_.count(binding) == 0 && "binding already exists");

    ::vk::DescriptorSetLayoutBinding layoutBinding;
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = stageFlags;
    descriptorSetLayoutBindings_[binding] = layoutBinding;
    return *this;
}

::std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::build()
{
    return ::std::make_unique<DescriptorSetLayout>(device_, descriptorSetLayoutBindings_);
}

DescriptorSetLayout::DescriptorSetLayout(
    ::core::Device &device,
    ::std::unordered_map<uint32_t, ::vk::DescriptorSetLayoutBinding> bindings)
    : device_(device), descriptorSetLayoutBindings_(bindings) {

    ::std::vector<::vk::DescriptorSetLayoutBinding> setLayoutBindings;
    for(auto& binding : descriptorSetLayoutBindings_)
    {
        setLayoutBindings.push_back(binding.second);
    }

    ::vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
    setLayoutCreateInfo.setBindings(setLayoutBindings);
    descriptorSetLayout_ = device_.logicalDevice().createDescriptorSetLayout(setLayoutCreateInfo);

}

DescriptorSetLayout::~DescriptorSetLayout()
{
    device_.logicalDevice().destroyDescriptorSetLayout(descriptorSetLayout_);
}

DescriptorPool::Builder& DescriptorPool::Builder::addPoolSize(::vk::DescriptorType type, uint32_t count)
{
    poolSizes_.push_back({type, count});
    return *this;
}

DescriptorPool::Builder& DescriptorPool::Builder::setPoolFlags(::vk::DescriptorPoolCreateFlags flags)
{
    flags_ = flags;
    return *this;
}

DescriptorPool::Builder& DescriptorPool::Builder::setMaxSets(uint32_t count)
{
    maxSets_ = count;
    return *this;
}

::std::unique_ptr<DescriptorPool> DescriptorPool::Builder::build()
{
    return ::std::make_unique<DescriptorPool>(device_, maxSets_, flags_, poolSizes_);
}

DescriptorPool::DescriptorPool(
    ::core::Device &device, uint32_t maxSets, ::vk::DescriptorPoolCreateFlags flags,
    ::std::vector<::vk::DescriptorPoolSize> poolSizes):device_(device) {
        ::vk::DescriptorPoolCreateInfo createInfo;
        createInfo.setPoolSizes(poolSizes)
                    .setMaxSets(maxSets)
                    .setFlags(flags);
        descriptorPool_ = device_.logicalDevice().createDescriptorPool(createInfo);
}

DescriptorPool::~DescriptorPool()
{
    device_.logicalDevice().destroyDescriptorPool(descriptorPool_);
}

void DescriptorPool::allocateDescriptor(const ::vk::DescriptorSetLayout &descriptorSetlayout, ::vk::DescriptorSet &descriptorSet)const
{
    ::vk::DescriptorSetAllocateInfo allocateInfo;
    allocateInfo.setDescriptorPool(descriptorPool_)
                .setSetLayouts(descriptorSetlayout);
    descriptorSet = device_.logicalDevice().allocateDescriptorSets(allocateInfo)[0];
}

void DescriptorPool::freeDescriptor(::std::vector<::vk::DescriptorSet>& descriptorSets)const
{
    device_.logicalDevice().freeDescriptorSets(descriptorPool_, descriptorSets);
}

void DescriptorPool::resetPool()
{
    device_.logicalDevice().resetDescriptorPool(descriptorPool_);
}

DescriptorWriter::DescriptorWriter(DescriptorSetLayout& descriptorSetLayout, DescriptorPool& pool):
                                        descriptorSetLayout_(descriptorSetLayout),pool_(pool){}

DescriptorWriter& DescriptorWriter::writeBuffer(uint32_t binding, ::vk::DescriptorBufferInfo &bufferInfo)
{
    auto &bindingDescrptor = descriptorSetLayout_.descriptorSetLayoutBindings_[binding];
    assert(bindingDescrptor.descriptorCount == 1 && "binding single descriptor info, but binding expects multiple");
    ::vk::WriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.setDescriptorType(bindingDescrptor.descriptorType)
                        .setDstBinding(binding)
                        .setBufferInfo(bufferInfo);
    writeDescriptorSets_.push_back(writeDescriptorSet);
    return *this;
}

DescriptorWriter& DescriptorWriter::writeImage(uint32_t binding, ::vk::DescriptorImageInfo imageInfo)
{
    auto &bindingDescrptor = descriptorSetLayout_.descriptorSetLayoutBindings_[binding];
    assert(bindingDescrptor.descriptorCount == 1 && "binding single descriptor info, but binding expects multiple");
    ::vk::WriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.setDescriptorType(bindingDescrptor.descriptorType)
                        .setDstBinding(binding)
                        .setImageInfo(imageInfo);
    writeDescriptorSets_.push_back(writeDescriptorSet);
    return *this;
}

void DescriptorWriter::build(::vk::DescriptorSet &descriptorSet)
{
    pool_.allocateDescriptor(descriptorSetLayout_.getDescriptorSetLayout(), descriptorSet);
    overwrite(descriptorSet);
}

void DescriptorWriter::overwrite(::vk::DescriptorSet &descriptorSet)
{
    for(auto& writeDescriptorSet : writeDescriptorSets_)
    {
        writeDescriptorSet.setDstSet(descriptorSet);
    }
    pool_.device_.logicalDevice().updateDescriptorSets(writeDescriptorSets_.size(), writeDescriptorSets_.data(), 0, nullptr);
}

}
