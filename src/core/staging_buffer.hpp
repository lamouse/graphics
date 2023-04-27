#pragma once
#include <vulkan/vulkan.hpp>
#include "device.hpp"
#include <functional>

namespace core {
    class StagingBuffer
    {
        private:
            ::vk::Buffer buffer;
            ::vk::DeviceMemory bufferMemory;
            ::vk::DeviceSize buff_size;
            ::std::reference_wrapper<::vk::Device> device_;
        public:
            StagingBuffer(Device& device, ::vk::DeviceSize size, const void* bufferData);
            StagingBuffer(const StagingBuffer&) = delete;
            StagingBuffer(StagingBuffer&&) = default;
            auto operator=(const StagingBuffer&) -> StagingBuffer = delete;
            auto operator=(StagingBuffer&&) -> StagingBuffer& = default;
            auto getBuffer() -> ::vk::Buffer{return buffer;}
            ~StagingBuffer();

    };
}