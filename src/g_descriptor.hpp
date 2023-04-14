#pragma once
#include <vulkan/vulkan.hpp>

namespace g {
    class Descriptor
    {
        private:
            ::vk::DescriptorPool descriptorPool_;
            ::vk::DescriptorSetLayout descriptorSetLayout_;
            bool is_init = false;
            bool is_destroy = false;
        void createDescriptorSetLayout(::vk::Device& device);
        void createDescriptorPool(::vk::Device& device, uint32_t poolSize);
        public:
            Descriptor(const Descriptor&) = delete;
            Descriptor(Descriptor&&) = default;
            Descriptor operator=(const Descriptor&) = delete;
            Descriptor& operator=(Descriptor&&) = default;
            Descriptor() = default;
            void init(::vk::Device& device, uint32_t poolSize = 100);
            void destory(::vk::Device& device);
            
            ::vk::DescriptorSetLayout& getDescriptorSetLayout(){return descriptorSetLayout_;}
            ::vk::DescriptorPool& getDescriptorPool(){return descriptorPool_;}
            ~Descriptor();
    };

}