#pragma once

#include "core/device.hpp"

namespace g {
class Compute {
    private:
        ::vk::Pipeline computePipeline_;
        ::core::Device& device_;
        ::vk::PipelineLayout computePipelineLayout_;
        ::std::vector<::vk::CommandBuffer> commandBuffers_;
        ::std::vector<::vk::Semaphore> finishedSemaphores_;
        ::std::vector<::vk::Fence> inFlightFences_;
        void createCommandBuffers(uint32_t inFlightCount);
        void createSyncs(uint32_t inFlightCount);
        void createComputePipeline(::std::string& path);
        void createComputePipelineLayout(::vk::DescriptorSetLayout descriptorSetLayout);

    public:
        Compute(::core::Device& device, ::std::string& path, ::vk::DescriptorSetLayout descriptorSetLayout);
        void beginCompute(uint32_t currentFrameIndex);
        auto compute(uint32_t currentFrameIndex, ::vk::DescriptorSet descriptorSet) -> ::vk::CommandBuffer&;
        void endCompute(uint32_t currentFrameIndex);
        void init(uint32_t inFlightCount);
        ~Compute();
};
}  // namespace g