#pragma once
#include <memory>
#include <unordered_map>

#include "core/device.hpp"

namespace g {

class DescriptorSetLayout {
    public:
        class Builder {
            private:
                ::std::unordered_map<uint32_t, ::vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings_;

            public:
                auto addBinding(uint32_t binding, ::vk::DescriptorType type, ::vk::ShaderStageFlags stageFlags,
                                uint32_t count = 1) -> Builder &;
                auto build(core::Device &device) -> ::std::unique_ptr<DescriptorSetLayout>;
        };

        DescriptorSetLayout(core::Device &device,
                            ::std::unordered_map<uint32_t, ::vk::DescriptorSetLayoutBinding> bindings);
        ~DescriptorSetLayout();
        DescriptorSetLayout(const DescriptorSetLayout &) = delete;
        DescriptorSetLayout(DescriptorSetLayout &&) = delete;
        auto operator=(const DescriptorSetLayout &) -> DescriptorSetLayout & = delete;
        auto operator=(DescriptorSetLayout &&) -> DescriptorSetLayout & = delete;
        auto operator()()const { return descriptorSetLayout_; }
        auto operator()(uint32_t binding) -> ::vk::DescriptorSetLayoutBinding {
            return descriptorSetLayoutBindings_[binding];
        }
        friend class DescriptorWriter;

    private:
        ::vk::Device device_;
        ::vk::DescriptorSetLayout descriptorSetLayout_;
        ::std::unordered_map<uint32_t, ::vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings_;
};

class DescriptorPool {
    public:
        class Builder {
            private:
                uint32_t maxSets_ = 1000;
                ::vk::DescriptorPoolCreateFlags flags_;
                ::std::vector<::vk::DescriptorPoolSize> poolSizes_;

            public:
                auto addPoolSize(::vk::DescriptorType type, uint32_t count) -> Builder &;
                auto setPoolFlags(::vk::DescriptorPoolCreateFlags flags) -> Builder &;
                auto setMaxSets(uint32_t maxSets) -> Builder &;
                auto build(core::Device &device) -> ::std::unique_ptr<DescriptorPool>;
        };
        DescriptorPool(core::Device &device, uint32_t maxSets, ::vk::DescriptorPoolCreateFlags flags,
                       ::std::vector<::vk::DescriptorPoolSize> poolSizes);
        ~DescriptorPool();
        DescriptorPool(const DescriptorPool &) = delete;
        DescriptorPool(DescriptorPool &&) = delete;
        auto operator=(const DescriptorPool &) -> DescriptorPool & = delete;
        auto operator=(DescriptorPool &&) -> DescriptorPool & = delete;

        auto allocateDescriptor(const ::vk::DescriptorSetLayout &descriptorSetLayout) -> ::vk::DescriptorSet;
        void freeDescriptor(::std::vector<::vk::DescriptorSet> &descriptorSets) const;
        void resetPool() const;
        auto getDescriptorPool() -> ::vk::DescriptorPool & { return descriptorPool_; }
        friend class DescriptorWriter;

    private:
        ::vk::Device device_;
        ::vk::DescriptorPool descriptorPool_;
};

class DescriptorWriter {
    public:
        DescriptorWriter() = default;
        auto writeBuffer(const ::vk::DescriptorSetLayoutBinding &descriptorSetLayoutBinding,
                         ::vk::DescriptorBufferInfo &bufferInfo) -> DescriptorWriter &;
        auto writeImage(const ::vk::DescriptorSetLayoutBinding &descriptorSetLayoutBinding,
                        ::vk::DescriptorImageInfo imageInfo) -> DescriptorWriter &;
        auto build(DescriptorPool &pool, const ::vk::DescriptorSetLayout &descriptorSetLayout) -> ::vk::DescriptorSet;

    private:
        // DescriptorSetLayout &descriptorSetLayout_;
        // DescriptorPool &pool_;
        std::vector<::vk::WriteDescriptorSet> writeDescriptorSets_;
        void overwrite(const vk::Device &device, const ::vk::DescriptorSet &descriptorSet);
};

}  // namespace g