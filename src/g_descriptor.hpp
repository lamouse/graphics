#pragma once
#include <vulkan/vulkan.hpp>
#include <functional>
namespace g {
    class Descriptor
    {
        private:
            ::vk::DescriptorPool descriptorPool_;
            ::std::reference_wrapper<::vk::Device> device_;
            void createDescriptorPool(uint32_t count);
        public:
            Descriptor(const Descriptor&) = delete;
            Descriptor(Descriptor&&) = default;
            Descriptor operator=(const Descriptor&) = delete;
            Descriptor& operator=(Descriptor&&) = default;
            Descriptor(::vk::Device device, uint32_t poolSize = 100);
            ::vk::DescriptorPool getDescriptorPool(){return descriptorPool_;}
            ~Descriptor();
    };

}