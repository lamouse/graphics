module;
#include <span>
#include <vulkan/vulkan.hpp>
#include "common/common_types.hpp"
module render.vulkan.utils;
namespace render::vulkan::present::utils {
namespace {
constexpr auto PIPELINE_COLOR_WRITE_MASK =
    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
auto CreateWrappedPipelineImpl(const Device& device, vk::Format format, PipelineLayout& layout,
                               std::tuple<ShaderModule&, ShaderModule&> shaders,
                               vk::PipelineColorBlendAttachmentState blending) -> Pipeline {
    const std::array shader_stages{vk::PipelineShaderStageCreateInfo()
                                       .setStage(vk::ShaderStageFlagBits::eVertex)
                                       .setModule(*std::get<0>(shaders))
                                       .setPName("main"),

                                   vk::PipelineShaderStageCreateInfo()
                                       .setStage(vk::ShaderStageFlagBits::eFragment)
                                       .setModule(*std::get<1>(shaders))
                                       .setPName("main")};

    constexpr vk::PipelineVertexInputStateCreateInfo vertex_input_ci;

    constexpr auto input_assembly_ci = vk::PipelineInputAssemblyStateCreateInfo().setTopology(
        vk::PrimitiveTopology::eTriangleStrip);

    constexpr auto viewport_state_ci =
        vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1);

    constexpr auto rasterization_ci = vk::PipelineRasterizationStateCreateInfo()
                                          .setDepthClampEnable(VK_FALSE)
                                          .setRasterizerDiscardEnable(VK_FALSE)
                                          .setPolygonMode(vk::PolygonMode::eFill)
                                          .setCullMode(vk::CullModeFlagBits::eNone)
                                          .setFrontFace(vk::FrontFace::eClockwise)
                                          .setDepthBiasEnable(VK_FALSE)
                                          .setLineWidth(1.f);

    constexpr auto multi_sampling_ci = vk::PipelineMultisampleStateCreateInfo();

    const vk::PipelineColorBlendStateCreateInfo color_blend_ci{
        {}, VK_FALSE, vk::LogicOp::eCopy, blending, {0.0f, 0.0f, 0.0f, 0.0f},
    };

    constexpr std::array dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    const auto dynamic_state_ci =
        vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamic_states);

    auto rendering_ci = vk::PipelineRenderingCreateInfo().setColorAttachmentFormats(format);

    const auto pipeline_ci = vk::GraphicsPipelineCreateInfo()
                                 .setStages(shader_stages)
                                 .setPVertexInputState(&vertex_input_ci)
                                 .setPInputAssemblyState(&input_assembly_ci)
                                 .setPViewportState(&viewport_state_ci)
                                 .setPRasterizationState(&rasterization_ci)
                                 .setPMultisampleState(&multi_sampling_ci)
                                 .setPColorBlendState(&color_blend_ci)
                                 .setPDynamicState(&dynamic_state_ci)
                                 .setLayout(*layout)
                                 .setPNext(&rendering_ci);

    return device.logical().createPipeline(pipeline_ci);
}
}  // namespace

auto CreateWrappedDescriptorSetLayout(const Device& device,
                                      std::initializer_list<vk::DescriptorType> types)
    -> DescriptorSetLayout {
    std::vector<vk::DescriptorSetLayoutBinding> bindings(types.size());
    for (size_t i = 0; i < types.size(); i++) {
        bindings[i] = vk::DescriptorSetLayoutBinding{
            static_cast<u32>(i),
            std::data(types)[i],
            1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr,
        };
    }

    return device.logical().createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo{{}, bindings});
}

auto CreateWrappedRenderPass(const Device& device, vk::Format format,
                             vk::ImageLayout initial_layout) -> RenderPass {
    const vk::AttachmentDescription attachment =
        vk::AttachmentDescription()
            .setFlags(vk::AttachmentDescriptionFlagBits::eMayAlias)
            .setFormat(format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(initial_layout == vk::ImageLayout::eUndefined
                           ? vk::AttachmentLoadOp::eDontCare
                           : vk::AttachmentLoadOp::eLoad)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eLoad)
            .setStencilStoreOp(vk::AttachmentStoreOp::eStore)
            .setInitialLayout(initial_layout)
            .setFinalLayout(vk::ImageLayout::eGeneral);

    constexpr vk::AttachmentReference color_attachment_ref =
        vk::AttachmentReference().setLayout(vk::ImageLayout::eGeneral);

    const vk::SubpassDescription subpass_description =
        vk::SubpassDescription()
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(color_attachment_ref);

    constexpr vk::SubpassDependency dependency =
        vk::SubpassDependency()
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead |
                              vk::AccessFlagBits::eColorAttachmentWrite);
    ;
    return device.logical().createRenderPass(vk::RenderPassCreateInfo()
                                                 .setAttachments(attachment)
                                                 .setSubpasses(subpass_description)
                                                 .setDependencies(dependency));
}

auto CreateWrappedPipelineLayout(const Device& device, DescriptorSetLayout& layout)
    -> PipelineLayout {
    ;
    return device.logical().createPipelineLayout(
        vk::PipelineLayoutCreateInfo().setSetLayoutCount(1).setPSetLayouts(layout.address()));
}

auto CreateWrappedPipeline(const Device& device, vk::Format format, PipelineLayout& layout,
                           std::tuple<ShaderModule&, ShaderModule&> shaders) -> Pipeline {
    constexpr vk::PipelineColorBlendAttachmentState color_blend_attachment_disabled =
        vk::PipelineColorBlendAttachmentState()
            .setBlendEnable(VK_FALSE)
            .setSrcColorBlendFactor(vk::BlendFactor::eZero)
            .setDstColorBlendFactor(vk::BlendFactor::eZero)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eZero)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            .setAlphaBlendOp(vk::BlendOp::eAdd)
            .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    return CreateWrappedPipelineImpl(device, format, layout, shaders,
                                     color_blend_attachment_disabled);
}

auto CreateWrappedPremultipliedBlendingPipeline(const Device& device, vk::Format format,
                                                PipelineLayout& layout,
                                                std::tuple<ShaderModule&, ShaderModule&> shaders)
    -> Pipeline {
    constexpr vk::PipelineColorBlendAttachmentState color_blend_attachment_premultiplied =
        vk::PipelineColorBlendAttachmentState()
            .setBlendEnable(VK_TRUE)
            .setSrcColorBlendFactor(vk::BlendFactor::eOne)
            .setDstColorBlendFactor(vk::BlendFactor::eOneMinusConstantAlpha)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            .setAlphaBlendOp(vk::BlendOp::eAdd)
            .setColorWriteMask(PIPELINE_COLOR_WRITE_MASK);

    return CreateWrappedPipelineImpl(device, format, layout, shaders,
                                     color_blend_attachment_premultiplied);
}

auto CreateWrappedCoverageBlendingPipeline(const Device& device, vk::Format format,
                                           PipelineLayout& layout,
                                           std::tuple<ShaderModule&, ShaderModule&> shaders)
    -> Pipeline {
    constexpr vk::PipelineColorBlendAttachmentState color_blend_attachment_coverage =
        vk::PipelineColorBlendAttachmentState()
            .setBlendEnable(VK_TRUE)
            .setSrcColorBlendFactor(vk::BlendFactor::eSrc1Alpha)
            .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            .setAlphaBlendOp(vk::BlendOp::eAdd)
            .setColorWriteMask(PIPELINE_COLOR_WRITE_MASK);

    return CreateWrappedPipelineImpl(device, format, layout, shaders,
                                     color_blend_attachment_coverage);
}

auto CreateWrappedImage(MemoryAllocator& allocator, vk::Extent2D dimensions, vk::Format format)
    -> Image {
    const VkImageCreateInfo image_ci{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = static_cast<VkFormat>(format),
        .extent = {.width = dimensions.width, .height = dimensions.height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    return allocator.createImage(image_ci);
}

auto CreateWrappedImageView(const Device& device, Image& image, vk::Format format) -> ImageView {
    return device.logical().CreateImageView(vk::ImageViewCreateInfo{
        {},
        *image,
        vk::ImageViewType::e2D,
        format,
        {},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
}

auto CreateWrappedFramebuffer(const Device& device, RenderPass& render_pass, ImageView& dest_image,
                              vk::Extent2D extent) -> VulkanFramebuffer {
    return device.logical().createFramerBuffer(vk::FramebufferCreateInfo{

        {},
        *render_pass,
        1,
        dest_image.address(),
        extent.width,
        extent.height,
        1,
    });
}

auto CreateBilinearSampler(const Device& device) -> Sampler {
    const VkSamplerCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    return device.logical().CreateSampler(ci);
}

auto CreateWrappedDescriptorPool(const Device& device, size_t max_descriptors, size_t max_sets,
                                 std::initializer_list<vk::DescriptorType> types)
    -> VulkanDescriptorPool {
    std::vector<vk::DescriptorPoolSize> pool_sizes(types.size());
    for (u32 i = 0; i < types.size(); i++) {
        pool_sizes[i] = vk::DescriptorPoolSize{
            std::data(types)[i],
            static_cast<u32>(max_descriptors),
        };
    }

    return device.logical().createDescriptorPool(vk::DescriptorPoolCreateInfo{
        {}, static_cast<u32>(max_sets), pool_sizes

    });
}

auto CreateWrappedDescriptorSets(VulkanDescriptorPool& pool,
                                 std::span<vk::DescriptorSetLayout> layouts) -> DescriptorSets {
    return pool.Allocate(vk::DescriptorSetAllocateInfo{
        *pool,
        layouts,
    });
}

auto CreateWriteDescriptorSet(std::vector<vk::DescriptorImageInfo>& images, vk::Sampler sampler,
                              vk::ImageView view, vk::DescriptorSet set, u32 binding)
    -> vk::WriteDescriptorSet {
    assert(images.capacity() > images.size());
    auto& image_info = images.emplace_back(sampler, view, vk::ImageLayout::eGeneral);

    return vk::WriteDescriptorSet{
        set,         binding, 0,       1, vk::DescriptorType::eCombinedImageSampler,
        &image_info, nullptr, nullptr,
    };
}

void ClearColorImage(vk::CommandBuffer& cmdbuf, vk::Image image) {
    static constexpr std::array<vk::ImageSubresourceRange, 1> subresources{{{
        vk::ImageAspectFlagBits::eColor,
        0,
        1,
        0,
        1,
    }}};
    TransitionImageLayout(cmdbuf, image, vk::ImageLayout::eGeneral, vk::ImageLayout::eUndefined);
    cmdbuf.clearColorImage(image, vk::ImageLayout::eGeneral, {}, subresources);
}

void TransitionImageLayout(vk::CommandBuffer& cmdbuf, vk::Image image,
                           vk::ImageLayout target_layout, vk::ImageLayout source_layout) {
    constexpr vk::AccessFlags flags{vk::AccessFlagBits::eColorAttachmentRead |
                                    vk::AccessFlagBits::eColorAttachmentWrite |
                                    vk::AccessFlagBits::eShaderRead};
    const vk::ImageMemoryBarrier barrier{

        flags,
        flags,
        source_layout,
        target_layout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange{
            vk::ImageAspectFlagBits::eColor,
            0,
            1,
            0,
            1,
        },
    };
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                           vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, barrier);
}

void BeginRenderPass(vk::CommandBuffer& cmdbuf, vk::RenderPass render_pass,
                     vk::Framebuffer framebuffer, vk::Extent2D extent) {
    const vk::RenderPassBeginInfo render_pass_bi{
        render_pass,
        framebuffer,
        {
            {},
            extent,
        },
        0,
        nullptr,
    };
    cmdbuf.beginRenderPass(render_pass_bi, vk::SubpassContents::eInline);

    const vk::Viewport viewport{
        0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f,
    };
    const vk::Rect2D scissor{
        {0, 0},
        extent,
    };
    cmdbuf.setViewport(0, viewport);
    cmdbuf.setScissor(0, scissor);
}

void BeginDynamicRendering(vk::CommandBuffer& cmdbuf, vk::ImageView image_view,
                           vk::Extent2D extent) {
    vk::RenderingAttachmentInfo colorAttachment =
        vk::RenderingAttachmentInfo()
            .setImageView(image_view)
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}));
    vk::RenderingInfo renderingInfo = vk::RenderingInfo()
                                          .setLayerCount(1)
                                          .setRenderArea(vk::Rect2D().setExtent(extent))
                                          .setColorAttachments(colorAttachment);
    cmdbuf.beginRendering(&renderingInfo);

    const vk::Viewport viewport = vk::Viewport()
                                      .setWidth(static_cast<float>(extent.width))
                                      .setHeight(static_cast<float>(extent.height))
                                      .setMaxDepth(1.f);
    const vk::Rect2D scissor = vk::Rect2D().setExtent(extent);
    cmdbuf.setViewport(0, viewport);
    cmdbuf.setScissor(0, scissor);
}

auto CreateNearestNeighborSampler(const Device& device) -> Sampler {
    const VkSamplerCreateInfo ci_nn{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    return device.logical().CreateSampler(ci_nn);
}

auto CreateWrappedBuffer(MemoryAllocator& allocator, vk::DeviceSize size, MemoryUsage usage)
    -> Buffer {
    const VkBufferCreateInfo dst_buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    return allocator.createBuffer(dst_buffer_info, usage);
}

void DownloadColorImage(vk::CommandBuffer& cmdbuf, vk::Image image, vk::Buffer buffer,
                        vk::Extent3D extent) {
    const vk::ImageMemoryBarrier read_barrier{
        vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eTransferRead,
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eTransferSrcOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange{
            vk::ImageAspectFlagBits::eColor,
            0,
            VK_REMAINING_MIP_LEVELS,
            0,
            VK_REMAINING_ARRAY_LAYERS,
        },
    };
    const vk::ImageMemoryBarrier image_write_barrier{
        {},
        vk::AccessFlagBits::eMemoryWrite,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::ImageLayout::eGeneral,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange{
            vk::ImageAspectFlagBits::eColor,
            0,
            VK_REMAINING_MIP_LEVELS,
            0,
            VK_REMAINING_ARRAY_LAYERS,
        },
    };
#undef MemoryBarrier
    static constexpr vk::MemoryBarrier memory_write_barrier{
        vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite,
    };
    const vk::BufferImageCopy copy{
        0,
        0,
        0,
        vk::ImageSubresourceLayers{
            vk::ImageAspectFlagBits::eColor,
            0,
            0,
            1,
        },
        {0, 0, 0},
        extent,
    };
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                           vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, read_barrier);
    cmdbuf.copyImageToBuffer(image, vk::ImageLayout::eTransferSrcOptimal, buffer, copy);
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                           vk::PipelineStageFlagBits::eAllCommands, {}, memory_write_barrier,
                           nullptr, image_write_barrier);
}
}  // namespace render::vulkan::present::utils