#include "texture_cache.hpp"
#include "vulkan_common/device.hpp"
#include "scheduler.hpp"
#include "staging_buffer_pool.hpp"
#include "common/bit_util.h"
#undef min
#undef max
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
}  // namespace

TextureCacheRuntime::TextureCacheRuntime(const Device& device_, scheduler::Scheduler& scheduler_,
                                         MemoryAllocator& memory_allocator_,
                                         StagingBufferPool& staging_buffer_pool_,
                                         RenderPassCache& render_pass_cache_,
                                         resource::DescriptorPool& descriptor_pool,
                                         ComputePassDescriptorQueue& compute_pass_descriptor_queue)
    : device(device_),
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
    /*boost::container::small_vector<VkBufferImageCopy, 16> vk_in_copies(copies.size());
    boost::container::small_vector<VkBufferImageCopy, 16> vk_out_copies(copies.size());
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
        static constexpr VkMemoryBarrier READ_BARRIER{
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
        };
        static constexpr VkMemoryBarrier WRITE_BARRIER{
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
        };
        const std::array pre_barriers{
            VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                 VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = src_image,
                .subresourceRange =
                    src_range.SubresourceRange(static_cast<VkImageAspectFlags>(src_aspect_mask)),
            },
        };
        const std::array middle_in_barrier{
            VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = 0,
                .dstAccessMask = 0,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = src_image,
                .subresourceRange =
                    src_range.SubresourceRange(static_cast<VkImageAspectFlags>(src_aspect_mask)),
            },
        };
        const std::array middle_out_barrier{
            VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                 VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = dst_image,
                .subresourceRange =
                    dst_range.SubresourceRange(static_cast<VkImageAspectFlags>(dst_aspect_mask)),
            },
        };
        const std::array post_barriers{
            VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT |
                                 VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                 VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = dst_image,
                .subresourceRange =
                    dst_range.SubresourceRange(static_cast<VkImageAspectFlags>(dst_aspect_mask)),
            },
        };*/
    // TODO fix it
    // cmdbuf.pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    // VK_PIPELINE_STAGE_TRANSFER_BIT,
    //                        0, {}, {}, pre_barriers);

    // cmdbuf.CopyImageToBuffer(src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, copy_buffer,
    //                          vk_in_copies);
    // cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
    // VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    //                        0, WRITE_BARRIER, nullptr, middle_in_barrier);

    // cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    // VK_PIPELINE_STAGE_TRANSFER_BIT,
    //                        0, READ_BARRIER, {}, middle_out_barrier);
    // cmdbuf.copyBufferToImage(copy_buffer, dst_image, VK_IMAGE_LAYOUT_GENERAL, vk_out_copies);
    // cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
    // VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    //                        0, {}, {}, post_barriers);
    //});
}

}  // namespace render::vulkan