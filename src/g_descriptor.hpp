#pragma once
#include "core/device.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>
#include <unordered_map>


namespace g {

    class DescriptorSetLayout{
        public:
            class Builder{
                private:
                    core::Device& device_;
                    ::std::unordered_map<uint32_t, ::vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings_;

                public:
                    Builder(core::Device& device):device_(device){};
                    Builder& addBinding(uint32_t binding, ::vk::DescriptorType type, ::vk::ShaderStageFlags stageFlags, uint32_t count = 1);
                    ::std::unique_ptr<DescriptorSetLayout> build();
            };

            DescriptorSetLayout(core::Device& device, ::std::unordered_map<uint32_t, ::vk::DescriptorSetLayoutBinding> bindings);
            ~DescriptorSetLayout();
            DescriptorSetLayout(const DescriptorSetLayout&) = delete;
            DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
            ::vk::DescriptorSetLayout getDescriptorSetLayout(){return descriptorSetLayout_;}
            friend class DescriptorWriter;
        private:
            core::Device& device_;
            ::vk::DescriptorSetLayout descriptorSetLayout_;
            ::std::unordered_map<uint32_t, ::vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings_;
    };

    class DescriptorPool
    {
        public:
            class Builder
            {
                private:
                    core::Device& device_;
                    uint32_t maxSets_ = 1000;
                    ::vk::DescriptorPoolCreateFlags flags_;
                    ::std::vector<::vk::DescriptorPoolSize> poolSizes_;
                public:
                    Builder(core::Device& device):device_(device){};
                    Builder& addPoolSize(::vk::DescriptorType type, uint32_t count);
                    Builder& setPoolFlags(::vk::DescriptorPoolCreateFlags flags);
                    Builder& setMaxSets(uint32_t maxSets);
                    ::std:: unique_ptr<DescriptorPool> build();
            };
            DescriptorPool(core::Device& device, 
                            uint32_t maxSets, 
                            ::vk::DescriptorPoolCreateFlags flags,
                            ::std::vector<::vk::DescriptorPoolSize> poolSizes);
            ~DescriptorPool();
            DescriptorPool(const DescriptorPool&) = delete;
            DescriptorPool& operator=(const DescriptorPool&) = delete;
            void allocateDescriptor(const ::vk::DescriptorSetLayout& descriptorSetlayout, ::vk::DescriptorSet& descriptorSet) const;
            void freeDescriptor(::std::vector<::vk::DescriptorSet>& descriptorSets)const;
            void resetPool();
            ::vk::DescriptorPool& getDescriptorPool(){return descriptorPool_;}
            friend class DescriptorWriter;
        private:
            core::Device& device_;
            ::vk::DescriptorPool descriptorPool_;
    };

    class DescriptorWriter{
        public:
            DescriptorWriter(DescriptorSetLayout& descriptorSetLayout, DescriptorPool& pool);
            DescriptorWriter& writeBuffer(uint32_t binding, ::vk::DescriptorBufferInfo& bufferInfo);
            DescriptorWriter& writeImage(uint32_t binding, ::vk::DescriptorImageInfo imageInfo);
            void build(::vk::DescriptorSet& descriptorSet);
            void overwrite(::vk::DescriptorSet& descriptorSet);
        private:
            DescriptorSetLayout& descriptorSetLayout_;
            DescriptorPool &pool_;
            std::vector<::vk::WriteDescriptorSet> writeDescriptorSets_;
    };

}