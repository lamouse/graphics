#pragma once

#include "core/device.hpp"

namespace g{
    class Compute {
        private:
            ::vk::Pipeline computePipeline_;
            ::core::Device& device_;
            ::vk::PipelineLayout computePipelineLayout_;
            ::std::vector<::vk::CommandBuffer> commandBuffers_;
            ::std::vector<::vk::Semaphore> finishedSemaphores_;
            ::std::vector<::vk::Fence> inFlightFences_;
            void createCommandBuffers(int inFlightCount);
            void createSyncs(int inFlightCount);
            void createComputePipeline(::std::string& path);
            void createComputePipelineLayout(::vk::DescriptorSetLayout descriptorSetLayout);
        public:
            Compute(::core::Device& device, ::std::string& path, ::vk::DescriptorSetLayout descriptorSetLayout);
            void beginCompute(int currentFrameIndex);
            ::vk::CommandBuffer& compute(int currentFrameIndex, ::vk::DescriptorSet descriptorSet);
            void endCompute(int currentFrameIndex);
            void init(int inFlightCount);
            ~Compute();

    };
}