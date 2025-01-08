#pragma once
#include "vulkan_common/device.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"
namespace render::vulkan::present::utils {
auto CreateWrappedDescriptorSetLayout(const Device& device,
                                      std::initializer_list<vk::DescriptorType> types)
    -> DescriptorSetLayout;

auto CreateWrappedRenderPass(const Device& device, vk::Format format,
                             vk::ImageLayout initial_layout = vk::ImageLayout::eGeneral)
    -> RenderPass;

auto CreateWrappedPipelineLayout(const Device& device, DescriptorSetLayout& layout)
    -> PipelineLayout;

auto CreateWrappedPipeline(const Device& device, RenderPass& renderpass, PipelineLayout& layout,
                           std::tuple<ShaderModule&, ShaderModule&> shaders) -> Pipeline;

auto CreateWrappedPremultipliedBlendingPipeline(const Device& device, RenderPass& renderpass,
                                                PipelineLayout& layout,
                                                std::tuple<ShaderModule&, ShaderModule&> shaders)
    -> Pipeline;

auto CreateWrappedCoverageBlendingPipeline(const Device& device, RenderPass& renderpass,
                                           PipelineLayout& layout,
                                           std::tuple<ShaderModule&, ShaderModule&> shaders)
    -> Pipeline;
}  // namespace render::vulkan::present::utils