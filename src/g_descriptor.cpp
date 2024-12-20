#include "g_descriptor.hpp"

#include <cassert>
#include <ranges>
#include <utility>

#include "core/device.hpp"
namespace g {

auto DescriptorSetLayout::Builder::addBinding(uint32_t binding, ::vk::DescriptorType type,
                                              ::vk::ShaderStageFlags stageFlags,
                                              uint32_t count) -> DescriptorSetLayout::Builder & {
    assert(not descriptorSetLayoutBindings_.contains(binding) && "binding already exists");

    ::vk::DescriptorSetLayoutBinding layoutBinding;
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = stageFlags;
    descriptorSetLayoutBindings_[binding] = layoutBinding;
    return *this;
}

auto DescriptorSetLayout::Builder::build() -> ::std::unique_ptr<DescriptorSetLayout> {
    return ::std::make_unique<DescriptorSetLayout>(descriptorSetLayoutBindings_);
}

DescriptorSetLayout::DescriptorSetLayout(::std::unordered_map<uint32_t, ::vk::DescriptorSetLayoutBinding> bindings)
    : descriptorSetLayoutBindings_(std::move(bindings)) {
    core::Device device;
    ::std::vector<::vk::DescriptorSetLayoutBinding> setLayoutBindings;
    setLayoutBindings.reserve(descriptorSetLayoutBindings_.size());
    for (auto &v : descriptorSetLayoutBindings_ | std::views::values) {
        setLayoutBindings.push_back(v);
    }

    ::vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
    setLayoutCreateInfo.setBindings(setLayoutBindings);
    descriptorSetLayout_ = device.logicalDevice().createDescriptorSetLayout(setLayoutCreateInfo);
}

DescriptorSetLayout::~DescriptorSetLayout() {
    core::Device device;
    device.logicalDevice().destroyDescriptorSetLayout(descriptorSetLayout_);
}

auto DescriptorPool::Builder::addPoolSize(::vk::DescriptorType type, uint32_t count) -> DescriptorPool::Builder & {
    poolSizes_.emplace_back(type, count);
    return *this;
}

auto DescriptorPool::Builder::setPoolFlags(::vk::DescriptorPoolCreateFlags flags) -> DescriptorPool::Builder & {
    flags_ = flags;
    return *this;
}

auto DescriptorPool::Builder::setMaxSets(uint32_t maxSets) -> DescriptorPool::Builder & {
    maxSets_ = maxSets;
    return *this;
}

auto DescriptorPool::Builder::build() -> ::std::unique_ptr<DescriptorPool> {
    return ::std::make_unique<DescriptorPool>(maxSets_, flags_, poolSizes_);
}

DescriptorPool::DescriptorPool(uint32_t maxSets, ::vk::DescriptorPoolCreateFlags flags,
                               ::std::vector<::vk::DescriptorPoolSize> poolSizes) {
    core::Device device;
    ::vk::DescriptorPoolCreateInfo createInfo;
    createInfo.setPoolSizes(poolSizes).setMaxSets(maxSets).setFlags(flags);
    descriptorPool_ = device.logicalDevice().createDescriptorPool(createInfo);
}

DescriptorPool::~DescriptorPool() {
    core::Device device;
    device.logicalDevice().destroyDescriptorPool(descriptorPool_);
}

auto DescriptorPool::allocateDescriptor(const ::vk::DescriptorSetLayout &descriptorSetLayout) -> ::vk::DescriptorSet {
    ::vk::DescriptorSetAllocateInfo allocateInfo;
    allocateInfo.setDescriptorPool(descriptorPool_).setSetLayouts(descriptorSetLayout);
    core::Device device;
    return device.logicalDevice().allocateDescriptorSets(allocateInfo)[0];
}

void DescriptorPool::freeDescriptor(::std::vector<::vk::DescriptorSet> &descriptorSets) const {
    core::Device device;
    device.logicalDevice().freeDescriptorSets(descriptorPool_, descriptorSets);
}

void DescriptorPool::resetPool() const {
    core::Device device;
    device.logicalDevice().resetDescriptorPool(descriptorPool_);
}

auto DescriptorWriter::writeBuffer(const ::vk::DescriptorSetLayoutBinding &descriptorSetLayoutBinding,
                                   ::vk::DescriptorBufferInfo &bufferInfo) -> DescriptorWriter & {
    ::vk::WriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.setDescriptorType(descriptorSetLayoutBinding.descriptorType)
        .setDstBinding(descriptorSetLayoutBinding.binding)
        .setBufferInfo(bufferInfo);
    writeDescriptorSets_.push_back(writeDescriptorSet);
    return *this;
}

auto DescriptorWriter::writeImage(const ::vk::DescriptorSetLayoutBinding &descriptorSetLayoutBinding,
                                  ::vk::DescriptorImageInfo imageInfo) -> DescriptorWriter & {
    ::vk::WriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.setDescriptorType(descriptorSetLayoutBinding.descriptorType)
        .setDstBinding(descriptorSetLayoutBinding.binding)
        .setImageInfo(imageInfo);
    writeDescriptorSets_.push_back(writeDescriptorSet);
    return *this;
}

auto DescriptorWriter::build(DescriptorPool &pool,
                             const ::vk::DescriptorSetLayout &descriptorSetLayout) -> ::vk::DescriptorSet {
    auto descriptorSet = pool.allocateDescriptor(descriptorSetLayout);
    overwrite(descriptorSet);
    return descriptorSet;
}

void DescriptorWriter::overwrite(const ::vk::DescriptorSet &descriptorSet) {
    core::Device device;
    for (auto &writeDescriptorSet : writeDescriptorSets_) {
        writeDescriptorSet.setDstSet(descriptorSet);
    }
    device.logicalDevice().updateDescriptorSets(static_cast<uint32_t>(writeDescriptorSets_.size()),
                                                writeDescriptorSets_.data(), 0, nullptr);
}

}  // namespace g
