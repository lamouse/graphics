#include "texture_cache.hpp"
#include <spdlog/spdlog.h>
#include "vulkan_common/device.hpp"
#include "scheduler.hpp"
#include "staging_buffer_pool.hpp"
#include "common/bit_util.h"
#include "blit_image.hpp"
#undef min
#undef max
#undef MemoryBarrier

namespace render::vulkan {
namespace {
[[nodiscard]] VkOffset3D MakeOffset3D(texture::Offset3D offset3d) {
    return VkOffset3D{
        .x = offset3d.x,
        .y = offset3d.y,
        .z = offset3d.z,
    };
}

[[nodiscard]] VkExtent3D MakeExtent3D(texture::Extent3D extent3d) {
    return VkExtent3D{
        .width = static_cast<u32>(extent3d.width),
        .height = static_cast<u32>(extent3d.height),
        .depth = static_cast<u32>(extent3d.depth),
    };
}
[[nodiscard]] VkImageSubresourceLayers MakeImageSubresourceLayers(
    texture::SubresourceLayers subresource, VkImageAspectFlags aspect_mask) {
    return VkImageSubresourceLayers{
        .aspectMask = aspect_mask,
        .mipLevel = static_cast<u32>(subresource.base_level),
        .baseArrayLayer = static_cast<u32>(subresource.base_layer),
        .layerCount = static_cast<u32>(subresource.num_layers),
    };
}
[[nodiscard]] VkBufferImageCopy MakeBufferImageCopy(const texture::ImageCopy& copy, bool is_src,
                                                    VkImageAspectFlags aspect_mask) noexcept {
    return VkBufferImageCopy{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = MakeImageSubresourceLayers(
            is_src ? copy.src_subresource : copy.dst_subresource, aspect_mask),
        .imageOffset = MakeOffset3D(is_src ? copy.src_offset : copy.dst_offset),
        .imageExtent = MakeExtent3D(copy.extent),
    };
}
[[nodiscard]] VkImageAspectFlags ImageAspectMask(surface::PixelFormat format) {
    switch (surface::GetFormatType(format)) {
        case surface::SurfaceType::ColorTexture:
            return VK_IMAGE_ASPECT_COLOR_BIT;
        case surface::SurfaceType::Depth:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case surface::SurfaceType::Stencil:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        case surface::SurfaceType::DepthStencil:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            assert(false && "Invalid surface type");
            return VkImageAspectFlags{};
    }
}

[[nodiscard]] VkImageSubresourceLayers MakeSubresourceLayers(const TextureImageView* image_view) {
    return VkImageSubresourceLayers{
        .aspectMask = ImageAspectMask(image_view->format),
        .mipLevel = static_cast<u32>(image_view->range.base.level),
        .baseArrayLayer = static_cast<u32>(image_view->range.base.layer),
        .layerCount = static_cast<u32>(image_view->range.extent.layers),
    };
}

struct RangedBarrierRange {
        u32 min_mip = std::numeric_limits<u32>::max();
        u32 max_mip = std::numeric_limits<u32>::min();
        u32 min_layer = std::numeric_limits<u32>::max();
        u32 max_layer = std::numeric_limits<u32>::min();

        void AddLayers(const VkImageSubresourceLayers& layers) {
            min_mip = std::min(min_mip, layers.mipLevel);
            max_mip = std::max(max_mip, layers.mipLevel + 1);
            min_layer = std::min(min_layer, layers.baseArrayLayer);
            max_layer = std::max(max_layer, layers.baseArrayLayer + layers.layerCount);
        }

        VkImageSubresourceRange SubresourceRange(VkImageAspectFlags aspect_mask) const noexcept {
            return VkImageSubresourceRange{
                .aspectMask = aspect_mask,
                .baseMipLevel = min_mip,
                .levelCount = max_mip - min_mip,
                .baseArrayLayer = min_layer,
                .layerCount = max_layer - min_layer,
            };
        }
};

[[nodiscard]] auto MakeImageResolve(const texture::Region2D& dst_region,
                                    const texture::Region2D& src_region,
                                    const VkImageSubresourceLayers& dst_layers,
                                    const VkImageSubresourceLayers& src_layers)
    -> vk::ImageResolve {
    return VkImageResolve{
        .srcSubresource = src_layers,
        .srcOffset =
            {
                .x = src_region.start.x,
                .y = src_region.start.y,
                .z = 0,
            },
        .dstSubresource = dst_layers,
        .dstOffset =
            {
                .x = dst_region.start.x,
                .y = dst_region.start.y,
                .z = 0,
            },
        .extent =
            {
                .width = static_cast<u32>(dst_region.end.x - dst_region.start.x),
                .height = static_cast<u32>(dst_region.end.y - dst_region.start.y),
                .depth = 1,
            },
    };
}

[[nodiscard]] auto MakeImageBlit(const texture::Region2D& dst_region,
                                 const texture::Region2D& src_region,
                                 const VkImageSubresourceLayers& dst_layers,
                                 const VkImageSubresourceLayers& src_layers) -> vk::ImageBlit {
    return VkImageBlit{
        .srcSubresource = src_layers,
        .srcOffsets =
            {
                {
                    .x = src_region.start.x,
                    .y = src_region.start.y,
                    .z = 0,
                },
                {
                    .x = src_region.end.x,
                    .y = src_region.end.y,
                    .z = 1,
                },
            },
        .dstSubresource = dst_layers,
        .dstOffsets =
            {
                {
                    .x = dst_region.start.x,
                    .y = dst_region.start.y,
                    .z = 0,
                },
                {
                    .x = dst_region.end.x,
                    .y = dst_region.end.y,
                    .z = 1,
                },
            },
    };
}

}  // namespace

TextureCacheRuntime::TextureCacheRuntime(const Device& device_, scheduler::Scheduler& scheduler_,
                                         MemoryAllocator& memory_allocator_,
                                         StagingBufferPool& staging_buffer_pool_,
                                         RenderPassCache& render_pass_cache_,
                                         resource::DescriptorPool& descriptor_pool,
                                         ComputePassDescriptorQueue& compute_pass_descriptor_queue,
                                         BlitImageHelper& blit_image_helper_)
    : blit_image_helper(blit_image_helper_),
      device(device_),
      scheduler(scheduler_),
      memory_allocator(memory_allocator_),
      staging_buffer_pool(staging_buffer_pool_),
      render_pass_cache(render_pass_cache_) {
    if (device_.isStorageImageMultisampleSupported()) {
        msaa_copy_pass =
            std::make_unique<MSAACopyPass>(device_, scheduler_, descriptor_pool,
                                           staging_buffer_pool_, compute_pass_descriptor_queue);
    }
    if (!device.isKhrImageFormatListSupported()) {
        return;
    }

    for (size_t index_a = 0; index_a < surface::MaxPixelFormat; index_a++) {
        const auto image_format = static_cast<surface::PixelFormat>(index_a);
        if (IsPixelFormatASTC(image_format) && !device.isOptimalAstcSupported()) {
            view_formats[index_a].push_back(vk::Format::eA8B8G8R8UnormPack32);
        }
        for (size_t index_b = 0; index_b < surface::MaxPixelFormat; index_b++) {
            const auto view_format = static_cast<surface::PixelFormat>(index_b);
            const auto view_info = device_.surfaceFormat(FormatType::Optimal, true, view_format);
            view_formats[index_a].push_back(view_info.format);
        }
    }
}

void TextureCacheRuntime::Finish() { scheduler.finish(); }

StagingBufferRef TextureCacheRuntime::UploadStagingBuffer(size_t size) {
    return staging_buffer_pool.Request(size, MemoryUsage::Upload);
}

StagingBufferRef TextureCacheRuntime::DownloadStagingBuffer(size_t size, bool deferred) {
    return staging_buffer_pool.Request(size, MemoryUsage::Download, deferred);
}

void TextureCacheRuntime::FreeDeferredStagingBuffer(StagingBufferRef& ref) {
    staging_buffer_pool.FreeDeferred(ref);
}

auto TextureCacheRuntime::ShouldReinterpret(TextureImage& dst, TextureImage& src) -> bool {
    if (surface::GetFormatType(dst.info.format) == surface::SurfaceType::DepthStencil &&
        !device.isExtShaderStencilExportSupported()) {
        return true;
    }
    if (dst.info.format == surface::PixelFormat::D32_FLOAT_S8_UINT ||
        src.info.format == surface::PixelFormat::D32_FLOAT_S8_UINT) {
        return true;
    }
    return false;
}

vk::Buffer TextureCacheRuntime::GetTemporaryBuffer(size_t needed_size) {
    const auto level = (8 * sizeof(size_t)) - std::countl_zero(needed_size - 1ULL);
    if (buffers[level]) {
        return *buffers[level];
    }
    const auto new_size = common::NextPow2(needed_size);
    static constexpr VkBufferUsageFlags flags =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    const VkBufferCreateInfo temp_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = new_size,
        .usage = flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    buffers[level] = memory_allocator.createBuffer(temp_ci, MemoryUsage::DeviceLocal);
    return *buffers[level];
}

void TextureCacheRuntime::BarrierFeedbackLoop() {
    scheduler.requestOutsideRenderPassOperationContext();
}

void TextureCacheRuntime::ReinterpretImage(TextureImage& dst, TextureImage& src,
                                           std::span<const texture::ImageCopy> copies) {
    boost::container::small_vector<vk::BufferImageCopy, 16> vk_in_copies(copies.size());
    boost::container::small_vector<vk::BufferImageCopy, 16> vk_out_copies(copies.size());
    const vk::ImageAspectFlags src_aspect_mask = src.AspectMask();
    const vk::ImageAspectFlags dst_aspect_mask = dst.AspectMask();

    const auto bpp_in = BytesPerBlock(src.info.format) / DefaultBlockWidth(src.info.format);
    const auto bpp_out = BytesPerBlock(dst.info.format) / DefaultBlockWidth(dst.info.format);
    std::ranges::transform(copies, vk_in_copies.begin(),
                           [src_aspect_mask, bpp_in, bpp_out](const auto& copy) {
                               auto copy2 = copy;
                               copy2.src_offset.x = (bpp_out * copy.src_offset.x) / bpp_in;
                               copy2.extent.width = (bpp_out * copy.extent.width) / bpp_in;
                               return MakeBufferImageCopy(
                                   copy2, true, static_cast<VkImageAspectFlags>(src_aspect_mask));
                           });
    std::ranges::transform(copies, vk_out_copies.begin(), [dst_aspect_mask](const auto& copy) {
        return MakeBufferImageCopy(copy, false, static_cast<VkImageAspectFlags>(dst_aspect_mask));
    });
    const u32 img_bpp = BytesPerBlock(dst.info.format);
    size_t total_size = 0;
    for (const auto& copy : copies) {
        total_size += copy.extent.width * copy.extent.height * copy.extent.depth * img_bpp;
    }
    const VkBuffer copy_buffer = GetTemporaryBuffer(total_size);
    const VkImage dst_image = dst.Handle();
    const VkImage src_image = src.Handle();
    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([dst_image, src_image, copy_buffer, src_aspect_mask, dst_aspect_mask,
                      vk_in_copies, vk_out_copies](vk::CommandBuffer cmdbuf) {
        RangedBarrierRange dst_range;
        RangedBarrierRange src_range;
        for (const VkBufferImageCopy& copy : vk_in_copies) {
            src_range.AddLayers(copy.imageSubresource);
        }
        for (const VkBufferImageCopy& copy : vk_out_copies) {
            dst_range.AddLayers(copy.imageSubresource);
        }
        static constexpr vk::MemoryBarrier READ_BARRIER{
            vk::AccessFlagBits::eMemoryWrite,
            vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eTransferRead};
        static constexpr vk::MemoryBarrier WRITE_BARRIER{
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite};
        const std::array pre_barriers{
            vk::ImageMemoryBarrier{

                vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eColorAttachmentWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                    vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eTransferSrcOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                src_image,
                src_range.SubresourceRange(static_cast<VkImageAspectFlags>(src_aspect_mask)),
            },
        };
        const std::array middle_in_barrier{
            vk::ImageMemoryBarrier{
                {},
                {},
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                src_image,
                src_range.SubresourceRange(static_cast<VkImageAspectFlags>(src_aspect_mask)),
            },
        };
        const std::array middle_out_barrier{
            vk::ImageMemoryBarrier{
                vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eColorAttachmentWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                    vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eTransferWrite,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eTransferDstOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                dst_image,
                dst_range.SubresourceRange(static_cast<VkImageAspectFlags>(dst_aspect_mask)),
            },
        };
        const std::array post_barriers{
            vk::ImageMemoryBarrier{
                vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite |
                    vk::AccessFlagBits::eColorAttachmentRead |
                    vk::AccessFlagBits::eColorAttachmentWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentRead |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                    vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                dst_image,
                dst_range.SubresourceRange(static_cast<VkImageAspectFlags>(dst_aspect_mask)),
            },
        };
        // TODO fix it
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, pre_barriers);
        cmdbuf.copyImageToBuffer(src_image, vk::ImageLayout::eTransferSrcOptimal, copy_buffer,
                                 vk_in_copies);
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands, {}, WRITE_BARRIER, nullptr,
                               middle_in_barrier);

        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlagBits::eTransfer, {}, READ_BARRIER, {},
                               middle_out_barrier);
        cmdbuf.copyBufferToImage(copy_buffer, dst_image, vk::ImageLayout::eGeneral, vk_out_copies);
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, post_barriers);
    });
}

void TextureCacheRuntime::BlitImage(TextureFramebuffer* dst_framebuffer, TextureImageView& dst,
                                    TextureImageView& src,
                                    const render::texture::Region2D& dst_region,
                                    const render::texture::Region2D& src_region) {
    const VkImageAspectFlags aspect_mask = ImageAspectMask(src.format);
    const bool is_dst_msaa = dst.Samples() != VK_SAMPLE_COUNT_1_BIT;
    const bool is_src_msaa = src.Samples() != VK_SAMPLE_COUNT_1_BIT;
    if (aspect_mask != ImageAspectMask(dst.format)) {
        assert(false && "Incompatible blit from format {} to {}\"src.format, dst.format");
        return;
    }
    if (aspect_mask == VK_IMAGE_ASPECT_COLOR_BIT && !is_src_msaa && !is_dst_msaa) {
        // blit_image_helper.BlitColor(dst_framebuffer, src.Handle(Shader::TextureType::Color2D),
        // TODO 实现                    dst_region, src_region, filter, operation);
        return;
    }
    assert(src.format == dst.format);
    if (aspect_mask == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
        const auto format = src.format;
        const auto can_blit_depth_stencil = [this, format] {
            switch (format) {
                case surface::PixelFormat::D24_UNORM_S8_UINT:
                case surface::PixelFormat::S8_UINT_D24_UNORM:
                    return device.isBlitDepth24Stencil8Supported();
                case surface::PixelFormat::D32_FLOAT_S8_UINT:
                    return device.isBlitDepth32Stencil8Supported();
                default:
                    assert(false);
            }
        }();
        if (!can_blit_depth_stencil) {
            if (is_src_msaa || is_dst_msaa) {
                // blit_image_helper.BlitDepthStencil(dst_framebuffer, src.DepthView(),
                // src.StencilView(),
                //  TODO 实现                 dst_region, src_region, filter, operation);
                // throw std::runtime_error("is_src_msaa || is_dst_msaa");
            }
            return;
        }
    }
    assert(!(is_dst_msaa && !is_src_msaa));

    const vk::Image dst_image = dst.ImageHandle();
    const vk::Image src_image = src.ImageHandle();
    const VkImageSubresourceLayers dst_layers = MakeSubresourceLayers(&dst);
    const VkImageSubresourceLayers src_layers = MakeSubresourceLayers(&src);
    const bool is_resolve = is_src_msaa && !is_dst_msaa;
    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([dst_region, src_region, dst_image, src_image, dst_layers, src_layers,
                      aspect_mask, is_resolve](vk::CommandBuffer cmdbuf) {
        const std::array read_barriers{
            vk::ImageMemoryBarrier{
                vk::AccessFlagBits::eShaderWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                    vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                src_image,
                vk::ImageSubresourceRange{
                    static_cast<vk::ImageAspectFlags>(aspect_mask),
                    0,
                    VK_REMAINING_MIP_LEVELS,
                    0,
                    VK_REMAINING_ARRAY_LAYERS,
                },
            },
            vk::ImageMemoryBarrier{
                vk::AccessFlagBits::eShaderWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                    vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eGeneral,
                vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED, dst_image,
                vk::ImageSubresourceRange{static_cast<vk::ImageAspectFlags>(aspect_mask), 0,
                                          VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS}},
        };
        vk::ImageMemoryBarrier write_barrier{
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite |
                vk::AccessFlagBits::eDepthStencilAttachmentRead |
                vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            dst_image,
            vk::ImageSubresourceRange{
                static_cast<vk::ImageAspectFlags>(aspect_mask),
                0,
                VK_REMAINING_MIP_LEVELS,
                0,
                VK_REMAINING_ARRAY_LAYERS,
            },
        };
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr,
                               read_barriers);
        if (is_resolve) {
            cmdbuf.resolveImage(src_image, vk::ImageLayout::eGeneral, dst_image,
                                vk::ImageLayout::eTransferDstOptimal,
                                MakeImageResolve(dst_region, src_region, dst_layers, src_layers));
        } else {
            // TODO 这里可以添加配置
            const bool is_linear = true;
            const vk::Filter vk_filter = is_linear ? vk::Filter::eLinear : vk::Filter::eNearest;
            cmdbuf.blitImage(src_image, vk::ImageLayout::eGeneral, dst_image,
                             vk::ImageLayout::eTransferDstOptimal,
                             MakeImageBlit(dst_region, src_region, dst_layers, src_layers),
                             vk_filter);
        }
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, write_barrier);
    });
}

void TextureCacheRuntime::ConvertImage(TextureFramebuffer* dst, TextureImageView& dst_view,
                                       TextureImageView& src_view) {
    switch (dst_view.format) {
        case surface::PixelFormat::R16_UNORM:
            if (src_view.format == surface::PixelFormat::D16_UNORM) {
                return blit_image_helper.ConvertD16ToR16(dst, src_view);
            }
            break;
        case surface::PixelFormat::A8B8G8R8_SRGB:
            if (src_view.format == surface::PixelFormat::D32_FLOAT) {
                return blit_image_helper.ConvertD32FToABGR8(dst, src_view);
            }
            break;
        case surface::PixelFormat::A8B8G8R8_UNORM:
            if (src_view.format == surface::PixelFormat::S8_UINT_D24_UNORM) {
                return blit_image_helper.ConvertD24S8ToABGR8(dst, src_view);
            }
            if (src_view.format == surface::PixelFormat::D24_UNORM_S8_UINT) {
                return blit_image_helper.ConvertS8D24ToABGR8(dst, src_view);
            }
            if (src_view.format == surface::PixelFormat::D32_FLOAT) {
                return blit_image_helper.ConvertD32FToABGR8(dst, src_view);
            }
            break;
        case surface::PixelFormat::B8G8R8A8_SRGB:
            if (src_view.format == surface::PixelFormat::D32_FLOAT) {
                return blit_image_helper.ConvertD32FToABGR8(dst, src_view);
            }
            break;
        case surface::PixelFormat::B8G8R8A8_UNORM:
            if (src_view.format == surface::PixelFormat::D32_FLOAT) {
                return blit_image_helper.ConvertD32FToABGR8(dst, src_view);
            }
            break;
        case surface::PixelFormat::R32_FLOAT:
            if (src_view.format == surface::PixelFormat::D32_FLOAT) {
                return blit_image_helper.ConvertD32ToR32(dst, src_view);
            }
            break;
        case surface::PixelFormat::D16_UNORM:
            if (src_view.format == surface::PixelFormat::R16_UNORM) {
                return blit_image_helper.ConvertR16ToD16(dst, src_view);
            }
            break;
        case surface::PixelFormat::S8_UINT_D24_UNORM:
            if (src_view.format == surface::PixelFormat::A8B8G8R8_UNORM ||
                src_view.format == surface::PixelFormat::B8G8R8A8_UNORM) {
                return blit_image_helper.ConvertABGR8ToD24S8(dst, src_view);
            }
            break;
        case surface::PixelFormat::D32_FLOAT:
            if (src_view.format == surface::PixelFormat::A8B8G8R8_UNORM ||
                src_view.format == surface::PixelFormat::B8G8R8A8_UNORM ||
                src_view.format == surface::PixelFormat::A8B8G8R8_SRGB ||
                src_view.format == surface::PixelFormat::B8G8R8A8_SRGB) {
                return blit_image_helper.ConvertABGR8ToD32F(dst, src_view);
            }
            if (src_view.format == surface::PixelFormat::R32_FLOAT) {
                return blit_image_helper.ConvertR32ToD32(dst, src_view);
            }
            break;
        default:
            break;
    }
    SPDLOG_WARN("Unimplemented format copy from to ");
}

}  // namespace render::vulkan