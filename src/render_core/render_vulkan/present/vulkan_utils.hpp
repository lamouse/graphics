#pragma once

#include <span>
#include <vulkan/vulkan.hpp>
#include "common/common_types.hpp"
import render.vulkan.common;
namespace render::vulkan::present::utils {
auto CreateWrappedDescriptorSetLayout(const Device& device,
                                      std::initializer_list<vk::DescriptorType> types)
    -> DescriptorSetLayout;

auto CreateWrappedRenderPass(const Device& device, vk::Format format,
                             vk::ImageLayout initial_layout = vk::ImageLayout::eGeneral)
    -> RenderPass;

auto CreateWrappedPipelineLayout(const Device& device, DescriptorSetLayout& layout)
    -> PipelineLayout;

auto CreateWrappedPipeline(const Device& device, vk::Format format, PipelineLayout& layout,
                           std::tuple<ShaderModule&, ShaderModule&> shaders) -> Pipeline;

auto CreateWrappedPremultipliedBlendingPipeline(const Device& device, vk::Format format,
                                                PipelineLayout& layout,
                                                std::tuple<ShaderModule&, ShaderModule&> shaders)
    -> Pipeline;

auto CreateWrappedCoverageBlendingPipeline(const Device& device, vk::Format format,
                                           PipelineLayout& layout,
                                           std::tuple<ShaderModule&, ShaderModule&> shaders)
    -> Pipeline;

auto CreateWrappedImage(MemoryAllocator& allocator, vk::Extent2D dimensions, vk::Format format)
    -> Image;

auto CreateWrappedImageView(const Device& device, Image& image, vk::Format format) -> ImageView;

auto CreateWrappedDescriptorPool(
    const Device& device, size_t max_descriptors, size_t max_sets,
    std::initializer_list<vk::DescriptorType> types = {vk::DescriptorType::eCombinedImageSampler})
    -> VulkanDescriptorPool;

auto CreateWrappedDescriptorSets(VulkanDescriptorPool& pool,
                                 std::span<vk::DescriptorSetLayout> layouts) -> DescriptorSets;

auto CreateWriteDescriptorSet(std::vector<vk::DescriptorImageInfo>& images, vk::Sampler sampler,
                              vk::ImageView view, vk::DescriptorSet set, u32 binding)
    -> vk::WriteDescriptorSet;

auto CreateWrappedFramebuffer(const Device& device, RenderPass& render_pass, ImageView& dest_image,
                              vk::Extent2D extent) -> VulkanFramebuffer;

auto CreateBilinearSampler(const Device& device) -> Sampler;

void ClearColorImage(vk::CommandBuffer& cmdbuf, vk::Image image);

void TransitionImageLayout(vk::CommandBuffer& cmdbuf, vk::Image image,
                           vk::ImageLayout target_layout,
                           vk::ImageLayout source_layout = vk::ImageLayout::eGeneral);

void BeginRenderPass(vk::CommandBuffer& cmdbuf, vk::RenderPass render_pass,
                     vk::Framebuffer framebuffer, vk::Extent2D extent);

void BeginDynamicRendering(vk::CommandBuffer& cmdbuf, vk::ImageView image_view,
                           vk::Extent2D extent);

auto CreateNearestNeighborSampler(const Device& device) -> Sampler;

auto CreateWrappedBuffer(MemoryAllocator& allocator, vk::DeviceSize size, MemoryUsage usage)
    -> Buffer;
void DownloadColorImage(vk::CommandBuffer& cmdbuf, vk::Image image, vk::Buffer buffer,
                        vk::Extent3D extent);

}  // namespace render::vulkan::present::utils