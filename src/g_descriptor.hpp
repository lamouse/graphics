#pragma once
#include <vulkan/vulkan.hpp>

namespace g {
    class Descriptor
    {
        private:
            ::vk::DescriptorPool descriptorPool_;
            bool is_init = false;
            bool is_destroy = false;
        public:
            Descriptor(const Descriptor&) = delete;
            Descriptor(Descriptor&&) = default;
            Descriptor operator=(const Descriptor&) = delete;
            Descriptor& operator=(Descriptor&&) = default;
            Descriptor() = default;
            void createDescriptorPool(::vk::Device& device, uint32_t poolSize = 100);
            void destoryDescriptorPool(::vk::Device& device);
            ::vk::DescriptorPool getDescriptorPool(){return descriptorPool_;}
            ~Descriptor();
    };

}