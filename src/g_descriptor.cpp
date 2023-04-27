#include "g_descriptor.hpp"

#include <iostream>
#include <cassert>
#include <utility>
namespace g {


auto DescriptorSetLayout::Builder::addBinding(
    uint32_t binding, ::vk::DescriptorType type,
    ::vk::ShaderStageFlags stageFlags, uint32_t count) -> DescriptorSetLayout::Builder& {
    assert(not descriptorSetLayoutBindings_.contains(binding) && "binding already exists");

    ::vk::DescriptorSetLayoutBinding layoutBinding;
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = stageFlags;
    descriptorSetLayoutBindings_[binding] = layoutBinding;
    return *this;
}

auto DescriptorSetLayout::Builder::build() -> ::std::unique_ptr<DescriptorSetLayout>
{
    return ::std::make_unique<DescriptorSetLayout>(device_, descriptorSetLayoutBindings_);
}

DescriptorSetLayout::DescriptorSetLayout(
    ::core::Device &device,
    ::std::unordered_map<uint32_t, ::vk::DescriptorSetLayoutBinding> bindings)
    : device_(device), descriptorSetLayoutBindings_(std::move(bindings)) {

    ::std::vector<::vk::DescriptorSetLayoutBinding> setLayoutBindings;
    setLayoutBindings.reserve(descriptorSetLayoutBindings_.size());
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

auto DescriptorPool::Builder::addPoolSize(::vk::DescriptorType type, uint32_t count) -> DescriptorPool::Builder&
{
    poolSizes_.emplace_back(type, count);
    return *this;
}

auto DescriptorPool::Builder::setPoolFlags(::vk::DescriptorPoolCreateFlags flags) -> DescriptorPool::Builder&
{
    flags_ = flags;
    return *this;
}

auto DescriptorPool::Builder::setMaxSets(uint32_t maxSets) -> DescriptorPool::Builder&
{
    maxSets_ = maxSets;
    return *this;
}

auto DescriptorPool::Builder::build() -> ::std::unique_ptr<DescriptorPool>
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

auto DescriptorWriter::writeBuffer(uint32_t binding, ::vk::DescriptorBufferInfo &bufferInfo) -> DescriptorWriter&
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

auto DescriptorWriter::writeImage(uint32_t binding, ::vk::DescriptorImageInfo imageInfo) -> DescriptorWriter&
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
