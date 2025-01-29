#include "texture_cache.hpp"
#include <spdlog/spdlog.h>
#include "vulkan_common/device.hpp"
#include "scheduler.hpp"
#include "staging_buffer_pool.hpp"
#include "common/bit_util.h"
#include "blit_image.hpp"
#include "texture/formatter.h"
#include "texture/samples_helper.h"
#include "common/settings.hpp"
#include "texture/util.hpp"
#include "render_core/compatible_formats.h"
#undef min
#undef max
#undef MemoryBarrier

namespace render::vulkan {
namespace {
[[nodiscard]] auto MakeOffset3D(texture::Offset3D offset3d) -> VkOffset3D {
    return VkOffset3D{
        .x = offset3d.x,
        .y = offset3d.y,
        .z = offset3d.z,
    };
}

constexpr auto ConvertBorderColor(const std::array<float, 4>& color) -> vk::BorderColor {
    if (color == std::array<float, 4>{0, 0, 0, 0}) {
        return vk::BorderColor::eFloatTransparentBlack;
    }
    if (color == std::array<float, 4>{0, 0, 0, 1}) {
        return vk::BorderColor::eFloatOpaqueBlack;
    }
    if (color == std::array<float, 4>{1, 1, 1, 1}) {
        return vk::BorderColor::eFloatOpaqueWhite;
    }
    if (color[0] + color[1] + color[2] > 1.35f) {
        // If color elements are brighter than roughly 0.5 average, use white border
        return vk::BorderColor::eFloatOpaqueWhite;
    }
    if (color[3] > 0.5f) {
        return vk::BorderColor::eFloatOpaqueBlack;
    }
    return vk::BorderColor::eFloatTransparentBlack;
}

auto samplerReduction(SamplerReduction reduction) -> vk::SamplerReductionMode {
    switch (reduction) {
        case SamplerReduction::WeightedAverage:
            return vk::SamplerReductionMode::eWeightedAverage;
        case SamplerReduction::Min:
            return vk::SamplerReductionMode::eMin;
        case SamplerReduction::Max:
            return vk::SamplerReductionMode::eMax;
    }
    SPDLOG_ERROR("Invalid sampler mode={}", static_cast<int>(reduction));
    return vk::SamplerReductionMode::eWeightedAverage;
}

[[nodiscard]] auto MakeExtent3D(texture::Extent3D extent3d) -> VkExtent3D {
    return VkExtent3D{
        .width = static_cast<u32>(extent3d.width),
        .height = static_cast<u32>(extent3d.height),
        .depth = static_cast<u32>(extent3d.depth),
    };
}
[[nodiscard]] vk::ImageSubresourceLayers MakeImageSubresourceLayers(
    texture::SubresourceLayers subresource, vk::ImageAspectFlags aspect_mask) {
    return vk::ImageSubresourceLayers{
        aspect_mask,
        static_cast<u32>(subresource.base_level),
        static_cast<u32>(subresource.base_layer),
        static_cast<u32>(subresource.num_layers),
    };
}
[[nodiscard]] auto MakeBufferImageCopy(const texture::ImageCopy& copy, bool is_src,
                                       vk::ImageAspectFlags aspect_mask) noexcept
    -> vk::BufferImageCopy {
    return vk::BufferImageCopy{
        0,
        0,
        0,
        MakeImageSubresourceLayers(is_src ? copy.src_subresource : copy.dst_subresource,
                                   aspect_mask),
        MakeOffset3D(is_src ? copy.src_offset : copy.dst_offset),
        MakeExtent3D(copy.extent),
    };
}
[[nodiscard]] auto ImageAspectMask(surface::PixelFormat format) -> vk::ImageAspectFlags {
    switch (surface::GetFormatType(format)) {
        case surface::SurfaceType::ColorTexture:
            return vk::ImageAspectFlagBits::eColor;
        case surface::SurfaceType::Depth:
            return vk::ImageAspectFlagBits::eDepth;
        case surface::SurfaceType::Stencil:
            return vk::ImageAspectFlagBits::eStencil;
        case surface::SurfaceType::DepthStencil:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        default:
            assert(false && "Invalid surface type");
            return vk::ImageAspectFlags{};
    }
}

[[nodiscard]] auto MakeSubresourceRange(vk::ImageAspectFlags aspect_mask,
                                        const texture::SubresourceRange& range)
    -> vk::ImageSubresourceRange {
    return vk::ImageSubresourceRange{
        aspect_mask,
        static_cast<u32>(range.base.level),
        static_cast<u32>(range.extent.levels),
        static_cast<u32>(range.base.layer),
        static_cast<u32>(range.extent.layers),
    };
}

[[nodiscard]] vk::ImageSubresourceLayers MakeSubresourceLayers(const TextureImageView* image_view) {
    return vk::ImageSubresourceLayers{
        ImageAspectMask(image_view->format),
        static_cast<u32>(image_view->range.base.level),
        static_cast<u32>(image_view->range.base.layer),
        static_cast<u32>(image_view->range.extent.layers),
    };
}

[[nodiscard]] auto MakeImageCopy(const texture::ImageCopy& copy,
                                 vk::ImageAspectFlags aspect_mask) noexcept -> vk::ImageCopy {
    return vk::ImageCopy{
        MakeImageSubresourceLayers(copy.src_subresource, aspect_mask),
        MakeOffset3D(copy.src_offset),
        MakeImageSubresourceLayers(copy.dst_subresource, aspect_mask),
        MakeOffset3D(copy.dst_offset),
        MakeExtent3D(copy.extent),
    };
}

[[nodiscard]] auto MakeStorageView(const LogicDevice& device, u32 level, vk::Image image,
                                   vk::Format format) -> ImageView {
    static constexpr vk::ImageViewUsageCreateInfo storage_image_view_usage_create_info{
        vk::ImageUsageFlagBits::eStorage};
    return device.CreateImageView(vk::ImageViewCreateInfo{
        {},
        image,
        vk::ImageViewType::e2DArray,
        format,
        vk::ComponentMapping{vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
                             vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, level, 1, 0,
                                  vk::RemainingArrayLayers},
        &storage_image_view_usage_create_info});
}

[[nodiscard]] auto TransformBufferImageCopies(std::span<const texture::BufferImageCopy> copies,
                                              size_t buffer_offset,
                                              vk::ImageAspectFlags aspect_mask)
    -> boost::container::small_vector<vk::BufferImageCopy, 16> {
    struct Maker {
            auto operator()(const texture::BufferImageCopy& copy) const -> vk::BufferImageCopy {
                return vk::BufferImageCopy{copy.buffer_offset + buffer_offset,
                                           copy.buffer_row_length,
                                           copy.buffer_image_height,
                                           vk::ImageSubresourceLayers{
                                               aspect_mask,
                                               static_cast<u32>(copy.image_subresource.base_level),
                                               static_cast<u32>(copy.image_subresource.base_layer),
                                               static_cast<u32>(copy.image_subresource.num_layers),
                                           },
                                           vk::Offset3D{
                                               copy.image_offset.x,
                                               copy.image_offset.y,
                                               copy.image_offset.z,
                                           },
                                           vk::Extent3D{
                                               copy.image_extent.width,
                                               copy.image_extent.height,
                                               copy.image_extent.depth,
                                           }};
            }
            size_t buffer_offset;
            vk::ImageAspectFlags aspect_mask;
    };

    if (aspect_mask == (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)) {
        boost::container::small_vector<vk::BufferImageCopy, 16> result(copies.size() * 2);
        std::ranges::transform(copies, result.begin(),
                               Maker{buffer_offset, vk::ImageAspectFlagBits::eDepth});
        std::ranges::transform(copies, result.begin() + copies.size(),
                               Maker{buffer_offset, vk::ImageAspectFlagBits::eStencil});
        return result;
    }
    boost::container::small_vector<vk::BufferImageCopy, 16> result(copies.size());
    std::ranges::transform(copies, result.begin(), Maker{buffer_offset, aspect_mask});
    return result;
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

        [[nodiscard]] auto SubresourceRange(vk::ImageAspectFlags aspect_mask) const noexcept
            -> vk::ImageSubresourceRange {
            return vk::ImageSubresourceRange{
                aspect_mask, min_mip, max_mip - min_mip, min_layer, max_layer - min_layer,
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
[[nodiscard]] auto SwapBlueRed(SwizzleSource value) -> SwizzleSource {
    switch (value) {
        case SwizzleSource::R:
            return SwizzleSource::B;
        case SwizzleSource::B:
            return SwizzleSource::R;
        default:
            return value;
    }
}

[[nodiscard]] auto SwapGreenRed(SwizzleSource value) -> SwizzleSource {
    switch (value) {
        case SwizzleSource::R:
            return SwizzleSource::G;
        case SwizzleSource::G:
            return SwizzleSource::R;
        default:
            return value;
    }
}

[[nodiscard]] auto SwapSpecial(SwizzleSource value) -> SwizzleSource {
    switch (value) {
        case SwizzleSource::A:
            return SwizzleSource::R;
        case SwizzleSource::R:
            return SwizzleSource::A;
        case SwizzleSource::G:
            return SwizzleSource::B;
        case SwizzleSource::B:
            return SwizzleSource::G;
        default:
            return value;
    }
}

[[nodiscard]] auto MakeSubresourceRange(const TextureImageView* image_view)
    -> vk::ImageSubresourceRange {
    texture::SubresourceRange range = image_view->range;
    if (True(image_view->flags & texture::ImageViewFlagBits::Slice)) {
        // Slice image views always affect a single layer, but their subresource range corresponds
        // to the slice. Override the value to affect a single layer.
        range.base.layer = 0;
        range.extent.layers = 1;
    }
    return MakeSubresourceRange(ImageAspectMask(image_view->format), range);
}

[[nodiscard]] auto Format(shader::ImageFormat format) -> vk::Format {
    switch (format) {
        case shader::ImageFormat::Typeless:
            break;
        case shader::ImageFormat::R8_SINT:
            return vk::Format::eR8Sint;
        case shader::ImageFormat::R8_UINT:
            return vk::Format::eR8Uint;
        case shader::ImageFormat::R16_UINT:
            return vk::Format::eR16Uint;
        case shader::ImageFormat::R16_SINT:
            return vk::Format::eR16Sint;
        case shader::ImageFormat::R32_UINT:
            return vk::Format::eR32Uint;
        case shader::ImageFormat::R32G32_UINT:
            return vk::Format::eR32G32Uint;
        case shader::ImageFormat::R32G32B32A32_UINT:
            return vk::Format::eR32G32B32A32Uint;
    }
    SPDLOG_ERROR("Invalid image format=");
    return vk::Format::eR32Uint;
}

[[nodiscard]] auto ImageViewType(shader::TextureType type) -> vk::ImageViewType {
    switch (type) {
        case shader::TextureType::Color1D:
            return vk::ImageViewType::e1D;
        case shader::TextureType::Color2D:
        case shader::TextureType::Color2DRect:
            return vk::ImageViewType::e2D;
        case shader::TextureType::ColorCube:
            return vk::ImageViewType::eCube;
        case shader::TextureType::Color3D:
            return vk::ImageViewType::e3D;
        case shader::TextureType::ColorArray1D:
            return vk::ImageViewType::e1DArray;
        case shader::TextureType::ColorArray2D:
            return vk::ImageViewType::e2DArray;
        case shader::TextureType::ColorArrayCube:
            return vk::ImageViewType::eCubeArray;
        case shader::TextureType::Buffer:
            SPDLOG_ERROR("Texture buffers can't be image views");
            return vk::ImageViewType::e1D;
    }
    SPDLOG_ERROR("Invalid image view type=");
    return vk::ImageViewType::e2D;
}

[[nodiscard]] auto ImageViewType(texture::ImageViewType type) -> vk::ImageViewType {
    switch (type) {
        case texture::ImageViewType::e1D:
            return vk::ImageViewType::e1D;
        case texture::ImageViewType::e2D:
        case texture::ImageViewType::Rect:
            return vk::ImageViewType::e2D;
        case texture::ImageViewType::Cube:
            return vk::ImageViewType::eCube;
        case texture::ImageViewType::e3D:
            return vk::ImageViewType::e3D;
        case texture::ImageViewType::e1DArray:
            return vk::ImageViewType::e1DArray;
        case texture::ImageViewType::e2DArray:
            return vk::ImageViewType::e2DArray;
        case texture::ImageViewType::CubeArray:
            return vk::ImageViewType::eCubeArray;
        case texture::ImageViewType::Buffer:
            SPDLOG_ERROR("Texture buffers can't be image views");
            return vk::ImageViewType::e1D;
    }
    SPDLOG_ERROR("Invalid image view type=");
    return vk::ImageViewType::e2D;
}

[[nodiscard]] SwizzleSource ConvertGreenRed(SwizzleSource value) {
    switch (value) {
        case SwizzleSource::G:
            return SwizzleSource::R;
        default:
            return value;
    }
}

[[nodiscard]] vk::ComponentSwizzle ComponentSwizzle(SwizzleSource swizzle) {
    switch (swizzle) {
        case SwizzleSource::Zero:
            return vk::ComponentSwizzle::eZero;
        case SwizzleSource::R:
            return vk::ComponentSwizzle::eR;
        case SwizzleSource::G:
            return vk::ComponentSwizzle::eG;
        case SwizzleSource::B:
            return vk::ComponentSwizzle::eB;
        case SwizzleSource::A:
            return vk::ComponentSwizzle::eA;
        case SwizzleSource::OneFloat:
        case SwizzleSource::OneInt:
            return vk::ComponentSwizzle::eOne;
    }
    SPDLOG_ERROR("Invalid swizzle");
    return vk::ComponentSwizzle::eZero;
}

void TryTransformSwizzleIfNeeded(surface::PixelFormat format, std::array<SwizzleSource, 4>& swizzle,
                                 bool emulate_bgr565, bool emulate_a4b4g4r4) {
    switch (format) {
        case surface::PixelFormat::A1B5G5R5_UNORM:
            std::ranges::transform(swizzle, swizzle.begin(), SwapBlueRed);
            break;
        case surface::PixelFormat::B5G6R5_UNORM:
            if (emulate_bgr565) {
                std::ranges::transform(swizzle, swizzle.begin(), SwapBlueRed);
            }
            break;
        case surface::PixelFormat::A5B5G5R1_UNORM:
            std::ranges::transform(swizzle, swizzle.begin(), SwapSpecial);
            break;
        case surface::PixelFormat::G4R4_UNORM:
            std::ranges::transform(swizzle, swizzle.begin(), SwapGreenRed);
            break;
        case surface::PixelFormat::A4B4G4R4_UNORM:
            if (emulate_a4b4g4r4) {
                std::ranges::reverse(swizzle);
            }
            break;
        default:
            break;
    }
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

[[nodiscard]] auto ImageUsageFlags(const FormatInfo& info, surface::PixelFormat format)
    -> vk::ImageUsageFlags {
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferSrc |
                                vk::ImageUsageFlagBits::eTransferDst |
                                vk::ImageUsageFlagBits::eSampled;
    if (info.attachable) {
        switch (surface::GetFormatType(format)) {
            case surface::SurfaceType::ColorTexture:
                usage |= vk::ImageUsageFlagBits::eColorAttachment;
                break;
            case surface::SurfaceType::Depth:
            case surface::SurfaceType::Stencil:
            case surface::SurfaceType::DepthStencil:
                usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
                break;
            default:
                assert(false && "Invalid surface type");
                break;
        }
    }
    if (info.storage) {
        usage |= vk::ImageUsageFlagBits::eStorage;
    }
    return usage;
}

[[nodiscard]] auto ConvertImageType(const texture::ImageType type) -> vk::ImageType {
    switch (type) {
        case texture::ImageType::e1D:
            return vk::ImageType::e1D;
        case texture::ImageType::e2D:
        case texture::ImageType::Linear:
            return vk::ImageType::e2D;
        case texture::ImageType::e3D:
            return vk::ImageType::e3D;
        case texture::ImageType::Buffer:
            break;
    }
    assert(false && "ConvertImageType Invalid image type=");
    return {};
}

[[nodiscard]] auto ConvertSampleCount(u32 num_samples) -> vk::SampleCountFlagBits {
    switch (num_samples) {
        case 1:
            return vk::SampleCountFlagBits::e1;
        case 2:
            return vk::SampleCountFlagBits::e2;
        case 4:
            return vk::SampleCountFlagBits::e4;
        case 8:
            return vk::SampleCountFlagBits::e8;
        case 16:
            return vk::SampleCountFlagBits::e16;
        default:
            SPDLOG_ERROR("Invalid number of samples={} will return vk::SampleCountFlagBits::e1",
                         num_samples);
            return vk::SampleCountFlagBits::e1;
    }
}

void CopyBufferToImage(vk::CommandBuffer cmdbuf, vk::Buffer src_buffer, vk::Image image,
                       vk::ImageAspectFlags aspect_mask, bool is_initialized,
                       std::span<const vk::BufferImageCopy> copies) {
    static constexpr vk::AccessFlags WRITE_ACCESS_FLAGS =
        vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    static constexpr vk::AccessFlags READ_ACCESS_FLAGS =
        vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentRead |
        vk::AccessFlagBits::eDepthStencilAttachmentRead;
    const vk::ImageMemoryBarrier read_barrier{
        WRITE_ACCESS_FLAGS,
        vk::AccessFlagBits::eTransferWrite,
        is_initialized ? vk::ImageLayout::eGeneral : vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange{
            aspect_mask,
            0,
            VK_REMAINING_MIP_LEVELS,
            0,
            VK_REMAINING_ARRAY_LAYERS,
        },
    };
    const vk::ImageMemoryBarrier write_barrier{
        vk::AccessFlagBits::eTransferWrite,
        WRITE_ACCESS_FLAGS | READ_ACCESS_FLAGS,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eGeneral,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange{
            aspect_mask,
            0,
            VK_REMAINING_MIP_LEVELS,
            0,
            VK_REMAINING_ARRAY_LAYERS,
        },
    };
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                           vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, read_barrier);
    cmdbuf.copyBufferToImage(src_buffer, image, vk::ImageLayout::eTransferDstOptimal, copies);
    // TODO: Move this to another API
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                           vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, write_barrier);
}

[[nodiscard]] auto MakeImageCreateInfo(const Device& device, const texture::ImageInfo& info)
    -> vk::ImageCreateInfo {
    const bool is_2d = (info.type == texture::ImageType::e2D);
    const bool is_3d = (info.type == texture::ImageType::e3D);
    const auto format_info = device.surfaceFormat(FormatType::Optimal, false, info.format);
    vk::ImageCreateFlags flags{};
    if (is_2d && info.resources.layers >= 6 && info.size.width == info.size.height &&
        !device.hasBrokenCubeImageCompatibility()) {
        flags |= vk::ImageCreateFlagBits::eCubeCompatible;
    }

    // fix moltenVK issues with some 3D games
    // credit to Jarrod Norwell from Sudachi https://github.com/jarrodnorwell/Sudachi
    auto usage = ImageUsageFlags(format_info, info.format);
    if (is_3d) {
        flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
        // Force usage to be VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT only on MoltenVK
        if (device.isMoltenVK()) {
            usage = vk::ImageUsageFlagBits::eColorAttachment;
        }
    }
    const auto [samples_x, samples_y] = texture::SamplesLog2(info.num_samples);
    return vk::ImageCreateInfo{flags,
                               ConvertImageType(info.type),
                               format_info.format,
                               vk::Extent3D{
                                   info.size.width >> samples_x,
                                   info.size.height >> samples_y,
                                   info.size.depth,
                               },
                               static_cast<u32>(info.resources.levels),
                               static_cast<u32>(info.resources.layers),
                               ConvertSampleCount(info.num_samples),
                               vk::ImageTiling::eOptimal,
                               usage,
                               vk::SharingMode::eExclusive,
                               0,
                               nullptr,
                               vk::ImageLayout::eUndefined};
}

[[nodiscard]] auto MakeImage(const Device& device, const MemoryAllocator& allocator,
                             const texture::ImageInfo& info,
                             std::span<const vk::Format> view_formats) -> Image {
    if (info.type == texture::ImageType::Buffer) {
        return Image{};
    }
    vk::ImageCreateInfo image_ci = MakeImageCreateInfo(device, info);
    const vk::ImageFormatListCreateInfo image_format_list{
        view_formats,
    };
    if (view_formats.size() > 1) {
        image_ci.flags |= vk::ImageCreateFlagBits::eMutableFormat;
        if (device.isKhrImageFormatListSupported()) {
            image_ci.pNext = &image_format_list;
        }
    }
    return allocator.createImage(image_ci);
}

void BlitScale(scheduler::Scheduler& scheduler, vk::Image src_image, vk::Image dst_image,
               const texture::ImageInfo& info, vk::ImageAspectFlags aspect_mask,
               const settings::ResolutionScalingInfo& resolution, bool up_scaling = true) {
    const bool is_2d = (info.type == texture::ImageType::e2D);
    const auto resources = info.resources;
    const VkExtent2D extent{
        .width = info.size.width,
        .height = info.size.height,
    };
    // Depth and integer formats must use NEAREST filter for blits.
    const bool is_color{aspect_mask == vk::ImageAspectFlagBits::eColor};
    const bool is_bilinear{is_color && !IsPixelFormatInteger(info.format)};
    const vk::Filter vk_filter = is_bilinear ? vk::Filter::eLinear : vk::Filter::eNearest;

    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([dst_image, src_image, extent, resources, aspect_mask, resolution, is_2d,
                      vk_filter, up_scaling](vk::CommandBuffer cmdbuf) {
        const vk::Offset2D src_size{
            static_cast<s32>(up_scaling ? extent.width : resolution.ScaleUp(extent.width)),
            static_cast<s32>(is_2d && up_scaling ? extent.height
                                                 : resolution.ScaleUp(extent.height)),
        };
        const vk::Offset2D dst_size{
            static_cast<s32>(up_scaling ? resolution.ScaleUp(extent.width) : extent.width),
            static_cast<s32>(is_2d && up_scaling ? resolution.ScaleUp(extent.height)
                                                 : extent.height),
        };
        boost::container::small_vector<vk::ImageBlit, 4> regions;
        regions.reserve(resources.levels);
        for (s32 level = 0; level < resources.levels; level++) {
            regions.push_back(
                vk::ImageBlit{vk::ImageSubresourceLayers{
                                  aspect_mask,
                                  static_cast<u32>(level),
                                  0,
                                  static_cast<u32>(resources.layers),
                              },
                              {
                                  vk::Offset3D{
                                      0,
                                      0,
                                      0,
                                  },
                                  vk::Offset3D{
                                      std::max(1, src_size.x >> level),
                                      std::max(1, src_size.y >> level),
                                      1,
                                  },
                              },
                              vk::ImageSubresourceLayers{
                                  aspect_mask,
                                  static_cast<u32>(level),
                                  0,
                                  static_cast<u32>(resources.layers),
                              },
                              {vk::Offset3D{0, 0, 0}, vk::Offset3D{
                                                          std::max(1, src_size.x >> level),
                                                          std::max(1, src_size.y >> level),
                                                          1,
                                                      }}});
        }
        const vk::ImageSubresourceRange subresource_range{aspect_mask, 0, VK_REMAINING_MIP_LEVELS,
                                                          0, VK_REMAINING_ARRAY_LAYERS};
        const std::array read_barriers{
            vk::ImageMemoryBarrier{
                vk::AccessFlagBits::eMemoryWrite,
                vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eTransferSrcOptimal,

                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                src_image,
                subresource_range,
            },
            vk::ImageMemoryBarrier{
                vk::AccessFlagBits::eShaderWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                    vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eTransferWrite,
                vk::ImageLayout::eUndefined,  // Discard contents
                vk::ImageLayout::eTransferDstOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                dst_image,
                subresource_range,
            },
        };
        const std::array write_barriers{
            vk::ImageMemoryBarrier{
                {},
                vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                src_image,
                subresource_range,
            },
            vk::ImageMemoryBarrier{
                vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                dst_image,
                subresource_range,
            },
        };
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, read_barriers);
        cmdbuf.blitImage(src_image, vk::ImageLayout::eTransferSrcOptimal, dst_image,
                         vk::ImageLayout::eTransferDstOptimal, regions, vk_filter);
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, write_barriers);
    });
}

[[nodiscard]] auto ImageViewAspectMask(const texture::ImageViewInfo& info) -> vk::ImageAspectFlags {
    if (info.IsRenderTarget()) {
        return ImageAspectMask(info.format);
    }
    bool any_r =
        std::ranges::any_of(info.Swizzle(), [](SwizzleSource s) { return s == SwizzleSource::R; });
    switch (info.format) {
        case surface::PixelFormat::D24_UNORM_S8_UINT:
        case surface::PixelFormat::D32_FLOAT_S8_UINT:
            // R = depth, G = stencil
            return any_r ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eStencil;
        case surface::PixelFormat::S8_UINT_D24_UNORM:
            // R = stencil, G = depth
            return any_r ? vk::ImageAspectFlagBits::eStencil : vk::ImageAspectFlagBits::eDepth;
        case surface::PixelFormat::D16_UNORM:
        case surface::PixelFormat::D32_FLOAT:
        case surface::PixelFormat::X8_D24_UNORM:
            return vk::ImageAspectFlagBits::eDepth;
        case surface::PixelFormat::S8_UINT:
            return vk::ImageAspectFlagBits::eStencil;
        default:
            return vk::ImageAspectFlagBits::eColor;
    }
}

}  // namespace

TextureCacheRuntime::TextureCacheRuntime(const Device& device_, scheduler::Scheduler& scheduler_,
                                         MemoryAllocator& memory_allocator_,
                                         StagingBufferPool& staging_buffer_pool_,
                                         RenderPassCache& render_pass_cache_,
                                         resource::DescriptorPool& descriptor_pool,
                                         ComputePassDescriptorQueue& compute_pass_descriptor_queue,
                                         BlitImageHelper& blit_image_helper_)
    : resolution(common::settings::get<settings::ResolutionScalingInfo>()),
      blit_image_helper(blit_image_helper_),
      device(device_),
      scheduler(scheduler_),
      memory_allocator(memory_allocator_),
      staging_buffer_pool(staging_buffer_pool_),
      render_pass_cache(render_pass_cache_) {
    if (common::settings::get<settings::Graphics>().astc_decodeMode ==
        settings::enums::AstcDecodeMode::Gpu) {
        astc_decoder_pass.emplace(device, scheduler, descriptor_pool, staging_buffer_pool,
                                  compute_pass_descriptor_queue, memory_allocator);
    }
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
            if (surface::IsViewCompatible(image_format, view_format, false, true)) {
                const auto view_info =
                    device_.surfaceFormat(FormatType::Optimal, true, view_format);
                view_formats[index_a].push_back(view_info.format);
            }
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
                               return MakeBufferImageCopy(copy2, true, src_aspect_mask);
                           });
    std::ranges::transform(copies, vk_out_copies.begin(), [dst_aspect_mask](const auto& copy) {
        return MakeBufferImageCopy(copy, false, dst_aspect_mask);
    });
    const u32 img_bpp = BytesPerBlock(dst.info.format);
    size_t total_size = 0;
    for (const auto& copy : copies) {
        total_size += copy.extent.width * copy.extent.height * copy.extent.depth * img_bpp;
    }
    const vk::Buffer copy_buffer = GetTemporaryBuffer(total_size);
    const vk::Image dst_image = dst.Handle();
    const vk::Image src_image = src.Handle();
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
                src_range.SubresourceRange(src_aspect_mask),
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
                src_range.SubresourceRange(src_aspect_mask),
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
                dst_range.SubresourceRange(dst_aspect_mask),
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
                dst_range.SubresourceRange(dst_aspect_mask),
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
    const vk::ImageAspectFlags aspect_mask = ImageAspectMask(src.format);
    const bool is_dst_msaa = dst.Samples() != vk::SampleCountFlagBits::e1;
    const bool is_src_msaa = src.Samples() != vk::SampleCountFlagBits::e1;
    if (aspect_mask != ImageAspectMask(dst.format)) {
        assert(false && "Incompatible blit from format {} to {}\"src.format, dst.format");
        return;
    }
    Operation opt = Operation::Blend;
    if (aspect_mask == vk::ImageAspectFlagBits::eColor && !is_src_msaa && !is_dst_msaa) {
        blit_image_helper.BlitColor(dst_framebuffer, src.Handle(shader::TextureType::Color2D),
                                    dst_region, src_region, opt);
        return;
    }
    assert(src.format == dst.format);
    if (aspect_mask == (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)) {
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
                blit_image_helper.BlitDepthStencil(dst_framebuffer, src.DepthView(),
                                                   src.StencilView(), dst_region, src_region, opt);
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

void TextureCacheRuntime::CopyImage(TextureImage& dst, TextureImage& src,
                                    std::span<const render::texture::ImageCopy> copies) {
    boost::container::small_vector<vk::ImageCopy, 16> vk_copies(copies.size());
    const vk::ImageAspectFlags aspect_mask = dst.AspectMask();
    assert(aspect_mask == src.AspectMask());

    std::ranges::transform(copies, vk_copies.begin(), [aspect_mask](const auto& copy) {
        return MakeImageCopy(copy, aspect_mask);
    });
    const vk::Image dst_image = dst.Handle();
    const vk::Image src_image = src.Handle();
    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([dst_image, src_image, aspect_mask, vk_copies](vk::CommandBuffer cmdbuf) {
        RangedBarrierRange dst_range;
        RangedBarrierRange src_range;
        for (const vk::ImageCopy& copy : vk_copies) {
            dst_range.AddLayers(copy.dstSubresource);
            src_range.AddLayers(copy.srcSubresource);
        }
        const std::array pre_barriers{
            vk::ImageMemoryBarrier{
                vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eColorAttachmentWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                    vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eTransferRead, vk::ImageLayout::eGeneral,
                vk::ImageLayout::eTransferSrcOptimal, VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED, src_image, src_range.SubresourceRange(aspect_mask)},
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
                dst_range.SubresourceRange(aspect_mask),
            },
        };
        const std::array post_barriers{
            vk::ImageMemoryBarrier{

                {},
                {},
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                src_image,
                src_range.SubresourceRange(aspect_mask),
            },
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
                dst_range.SubresourceRange(aspect_mask),
            },
        };
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, pre_barriers);
        cmdbuf.copyImage(src_image, vk::ImageLayout::eTransferSrcOptimal, dst_image,
                         vk::ImageLayout::eTransferDstOptimal, vk_copies);
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, post_barriers);
    });
}

void TextureCacheRuntime::CopyImageMSAA(TextureImage& dst, TextureImage& src,
                                        std::span<const texture::ImageCopy> copies) {
    const bool msaa_to_non_msaa = src.info.num_samples > 1 && dst.info.num_samples == 1;
    if (msaa_copy_pass) {
        return msaa_copy_pass->CopyImage(dst, src, copies, msaa_to_non_msaa);
    }
    SPDLOG_WARN("Copying images with different samples is not supported.");
}

auto TextureCacheRuntime::GetDeviceLocalMemory() const -> u64 {
    return device.getDeviceLocalMemory();
}

auto TextureCacheRuntime::GetDeviceMemoryUsage() const -> u64 {
    return device.getDeviceMemoryUsage();
}

auto TextureCacheRuntime::CanReportMemoryUsage() const -> bool {
    return device.canReportMemoryUsage();
}

void TextureCacheRuntime::TickFrame() {}

TextureImage::TextureImage(TextureCacheRuntime& runtime_, const texture::ImageInfo& info_)
    : texture::ImageBase(info_),
      scheduler{&runtime_.scheduler},
      runtime{&runtime_},
      original_image(MakeImage(runtime_.device, runtime_.memory_allocator, info,
                               runtime->ViewFormats(info.format))),
      aspect_mask(ImageAspectMask(info.format)) {
    if (IsPixelFormatASTC(info.format) && !runtime->device.isOptimalAstcSupported()) {
        switch (common::settings::get<settings::Graphics>().astc_decodeMode) {
            case settings::enums::AstcDecodeMode::Gpu:
                if (common::settings::get<settings::Graphics>().astc_recompression ==
                        settings::enums::AstcRecompression::Uncompressed &&
                    info.size.depth == 1) {
                    flags |= texture::ImageFlagBits::AcceleratedUpload;
                }
                break;
            case settings::enums::AstcDecodeMode::CpuAsynchronous:
                flags |= texture::ImageFlagBits::AsynchronousDecode;
                break;
            default:
                break;
        }
        flags |= texture::ImageFlagBits::Converted;
        flags |= texture::ImageFlagBits::CostlyLoad;
    }
    if (IsPixelFormatBCn(info.format) && !runtime->device.isOptimalBcnSupported()) {
        flags |= texture::ImageFlagBits::Converted;
        flags |= texture::ImageFlagBits::CostlyLoad;
    }
    if (runtime->device.hasDebuggingToolAttached()) {
        original_image.SetObjectNameEXT(texture::Name(*this).c_str());
    }
    current_image = &TextureImage::original_image;
    storage_image_views.resize(info.resources.levels);
    if (IsPixelFormatASTC(info.format) && !runtime->device.isOptimalAstcSupported() &&
        common::settings::get<settings::Graphics>().astc_recompression ==
            settings::enums::AstcRecompression::Uncompressed) {
        const auto& device = runtime->device.logical();
        for (s32 level = 0; level < info.resources.levels; ++level) {
            storage_image_views[level] =
                MakeStorageView(device, level, *original_image, vk::Format::eA8B8G8R8UnormPack32);
        }
    }
}

TextureImage::TextureImage(const texture::NullImageParams& params) : texture::ImageBase{params} {}
TextureImage::~TextureImage() = default;
void TextureImage::UploadMemory(vk::Buffer buffer, vk::DeviceSize offset,
                                std::span<const render::texture::BufferImageCopy> copies) {
    // TODO: Move this to another API
    const bool is_rescaled = True(flags & texture::ImageFlagBits::Rescaled);
    if (is_rescaled) {
        ScaleDown(true);
    }
    scheduler->requestOutsideRenderPassOperationContext();
    auto vk_copies = TransformBufferImageCopies(copies, offset, aspect_mask);
    const vk::Buffer src_buffer = buffer;
    const vk::Image vk_image = *original_image;
    const vk::ImageAspectFlags vk_aspect_mask = aspect_mask;
    const bool is_initialized = std::exchange(initialized, true);
    scheduler->record([src_buffer, vk_image, vk_aspect_mask, is_initialized,
                       vk_copies](vk::CommandBuffer cmdbuf) {
        CopyBufferToImage(cmdbuf, src_buffer, vk_image, vk_aspect_mask, is_initialized, vk_copies);
    });
    if (is_rescaled) {
        ScaleUp();
    }
}

void TextureImage::UploadMemory(const StagingBufferRef& map,
                                std::span<const texture::BufferImageCopy> copies) {
    UploadMemory(map.buffer, map.offset, copies);
}

void TextureImage::DownloadMemory(vk::Buffer buffer, size_t offset,
                                  std::span<const texture::BufferImageCopy> copies) {
    std::array buffer_handles{
        buffer,
    };
    std::array buffer_offsets{
        offset,
    };
    DownloadMemory(buffer_handles, buffer_offsets, copies);
}
auto TextureImage::StorageImageView(s32 level) noexcept -> vk::ImageView {
    auto& view = storage_image_views[level];
    if (!view) {
        const auto format_info =
            runtime->device.surfaceFormat(FormatType::Optimal, true, info.format);
        view = MakeStorageView(runtime->device.logical(), level, *(this->*current_image),
                               format_info.format);
    }
    return *view;
}

void TextureImage::DownloadMemory(std::span<vk::Buffer> buffers_span,
                                  std::span<size_t> offsets_span,
                                  std::span<const texture::BufferImageCopy> copies) {
    const bool is_rescaled = True(flags & texture::ImageFlagBits::Rescaled);
    if (is_rescaled) {
        ScaleDown();
    }
    boost::container::small_vector<vk::Buffer, 8> buffers_vector{};
    boost::container::small_vector<boost::container::small_vector<vk::BufferImageCopy, 16>, 8>
        vk_copies;
    for (size_t index = 0; index < buffers_span.size(); index++) {
        buffers_vector.emplace_back(buffers_span[index]);
        vk_copies.emplace_back(
            TransformBufferImageCopies(copies, offsets_span[index], aspect_mask));
    }
    scheduler->requestOutsideRenderPassOperationContext();
    scheduler->record([buffers = std::move(buffers_vector), image = *original_image,
                       aspect_mask_ = aspect_mask, vk_copies](vk::CommandBuffer cmdbuf) {
        const vk::ImageMemoryBarrier read_barrier{
            vk::AccessFlagBits::eMemoryWrite,
            vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eTransferSrcOptimal,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            image,
            vk::ImageSubresourceRange{
                aspect_mask_,
                0,
                VK_REMAINING_MIP_LEVELS,
                0,
                VK_REMAINING_ARRAY_LAYERS,
            },
        };
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, read_barrier);

        for (size_t index = 0; index < buffers.size(); index++) {
            cmdbuf.copyImageToBuffer(image, vk::ImageLayout::eTransferSrcOptimal, buffers[index],
                                     vk_copies[index]);
        }

        const vk::MemoryBarrier memory_write_barrier{
            vk::AccessFlagBits::eMemoryWrite,
            vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead};
        const vk::ImageMemoryBarrier image_write_barrier{

            {},
            vk::AccessFlagBits::eMemoryWrite,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            image,
            vk::ImageSubresourceRange{
                aspect_mask_,
                0,
                VK_REMAINING_MIP_LEVELS,
                0,
                VK_REMAINING_ARRAY_LAYERS,
            },
        };
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands, {}, memory_write_barrier,
                               {}, image_write_barrier);
    });
    if (is_rescaled) {
        ScaleUp(true);
    }
}

void TextureImage::DownloadMemory(const StagingBufferRef& map,
                                  std::span<const texture::BufferImageCopy> copies) {
    std::array buffers{
        map.buffer,
    };
    std::array offsets{
        static_cast<size_t>(map.offset),
    };
    DownloadMemory(buffers, offsets, copies);
}

auto TextureImage::IsRescaled() const noexcept -> bool {
    return True(flags & texture::ImageFlagBits::Rescaled);
}

auto TextureImage::ScaleUp(bool ignore) -> bool {
    const auto& resolution = runtime->resolution;
    if (!resolution.active) {
        return false;
    }
    if (True(flags & texture::ImageFlagBits::Rescaled)) {
        return false;
    }
    assert(info.type != texture::ImageType::Linear);
    flags |= texture::ImageFlagBits::Rescaled;
    has_scaled = true;
    if (!scaled_image) {
        const bool is_2d = (info.type == texture::ImageType::e2D);
        const u32 scaled_width = resolution.ScaleUp(info.size.width);
        const u32 scaled_height = is_2d ? resolution.ScaleUp(info.size.height) : info.size.height;
        auto scaled_info = info;
        scaled_info.size.width = scaled_width;
        scaled_info.size.height = scaled_height;
        scaled_image = MakeImage(runtime->device, runtime->memory_allocator, scaled_info,
                                 runtime->ViewFormats(info.format));
        ignore = false;
    }
    current_image = &TextureImage::scaled_image;
    if (ignore) {
        return true;
    }
    if (aspect_mask == vk::ImageAspectFlags{}) {
        aspect_mask = ImageAspectMask(info.format);
    }
    if (NeedsScaleHelper()) {
        return BlitScaleHelper(true);
    } else {
        BlitScale(*scheduler, *original_image, *scaled_image, info, aspect_mask, resolution);
    }
    return true;
}

bool TextureImage::ScaleDown(bool ignore) {
    const auto& resolution = runtime->resolution;
    if (!resolution.active) {
        return false;
    }
    if (False(flags & texture::ImageFlagBits::Rescaled)) {
        return false;
    }
    assert(info.type != texture::ImageType::Linear);
    flags &= ~texture::ImageFlagBits::Rescaled;
    current_image = &TextureImage::original_image;
    if (ignore) {
        return true;
    }
    if (aspect_mask == vk::ImageAspectFlags{}) {
        aspect_mask = ImageAspectMask(info.format);
    }
    if (NeedsScaleHelper()) {
        return BlitScaleHelper(false);
    } else {
        BlitScale(*scheduler, *scaled_image, *original_image, info, aspect_mask, resolution, false);
    }
    return true;
}

auto TextureImage::BlitScaleHelper(bool scale_up) -> bool {
    using namespace texture;
    static constexpr auto BLIT_OPERATION = Operation::SrcCopy;
    const bool is_color{aspect_mask == vk::ImageAspectFlagBits::eColor};
    const bool is_bilinear{is_color && !IsPixelFormatInteger(info.format)};
    // const auto operation = is_bilinear ? vk::Filter::eLinear : vk::Filter::eNearest;

    const bool is_2d = (info.type == ImageType::e2D);
    const auto& resolution = runtime->resolution;
    const u32 scaled_width = resolution.ScaleUp(info.size.width);
    const u32 scaled_height = is_2d ? resolution.ScaleUp(info.size.height) : info.size.height;
    std::unique_ptr<TextureImageView>& blit_view = scale_up ? scale_view : normal_view;
    std::unique_ptr<TextureFramebuffer>& blit_framebuffer =
        scale_up ? scale_framebuffer : normal_framebuffer;
    if (!blit_view) {
        const auto view_info = ImageViewInfo(ImageViewType::e2D, info.format);
        blit_view = std::make_unique<TextureImageView>(*runtime, view_info, NULL_IMAGE_ID, *this);
    }

    const u32 src_width = scale_up ? info.size.width : scaled_width;
    const u32 src_height = scale_up ? info.size.height : scaled_height;
    const u32 dst_width = scale_up ? scaled_width : info.size.width;
    const u32 dst_height = scale_up ? scaled_height : info.size.height;
    const Region2D src_region{
        .start = {0, 0},
        .end = {static_cast<s32>(src_width), static_cast<s32>(src_height)},
    };
    const Region2D dst_region{
        .start = {0, 0},
        .end = {static_cast<s32>(dst_width), static_cast<s32>(dst_height)},
    };
    const vk::Extent2D extent{
        std::max(scaled_width, info.size.width),
        std::max(scaled_height, info.size.height),
    };

    auto* view_ptr = blit_view.get();
    if (aspect_mask == vk::ImageAspectFlagBits::eColor) {
        if (!blit_framebuffer) {
            blit_framebuffer =
                std::make_unique<TextureFramebuffer>(*runtime, view_ptr, nullptr, extent, scale_up);
        }
        const auto color_view = blit_view->Handle(shader::TextureType::Color2D);

        runtime->blit_image_helper.BlitColor(blit_framebuffer.get(),
                                             static_cast<VkImageView>(color_view), dst_region,
                                             src_region, BLIT_OPERATION);  // BLIT_OPERATION
    } else if (aspect_mask ==
               (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)) {
        if (!blit_framebuffer) {
            blit_framebuffer =
                std::make_unique<TextureFramebuffer>(*runtime, nullptr, view_ptr, extent, scale_up);
        }
        runtime->blit_image_helper.BlitDepthStencil(
            blit_framebuffer.get(), static_cast<VkImageView>(blit_view->DepthView()),
            static_cast<VkImageView>(blit_view->StencilView()), dst_region, src_region,
            BLIT_OPERATION);  // BLIT_OPERATION
    } else {
        // TODO: Use helper blits where applicable
        flags &= ~ImageFlagBits::Rescaled;
        SPDLOG_ERROR("Device does not support scaling format {}", info.format);
        return false;
    }
    return true;
}

auto TextureImage::NeedsScaleHelper() const -> bool {
    const auto& device = runtime->device;
    const bool needs_msaa_helper = info.num_samples > 1 && device.cantBlitMSAA();
    if (needs_msaa_helper) {
        return true;
    }
    static constexpr auto OPTIMAL_FORMAT = FormatType::Optimal;
    const auto vk_format = device.surfaceFormat(OPTIMAL_FORMAT, false, info.format).format;
    const auto blit_usage =
        vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eBlitDst;
    const bool needs_blit_helper = !device.isFormatSupported(vk_format, blit_usage, OPTIMAL_FORMAT);
    return needs_blit_helper;
}

TextureImageView::TextureImageView(TextureCacheRuntime& runtime, const texture::ImageViewInfo& info,
                                   texture::ImageId image_id_, TextureImage& image)
    : texture::ImageViewBase{info, image.info, image_id_},
      device{&runtime.device},
      image_handle{image.Handle()},
      samples(ConvertSampleCount(image.info.num_samples)) {
    using shader::TextureType;

    const vk::ImageAspectFlags aspect_mask = ImageViewAspectMask(info);
    std::array<SwizzleSource, 4> swizzle{
        SwizzleSource::R,
        SwizzleSource::G,
        SwizzleSource::B,
        SwizzleSource::A,
    };
    if (!info.IsRenderTarget()) {
        swizzle = info.Swizzle();
        TryTransformSwizzleIfNeeded(format, swizzle, device->mustEmulateBGR565(),
                                    !device->isExt4444FormatsSupported());
        if ((aspect_mask & (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)) !=
            vk::ImageAspectFlags{}) {
            std::ranges::transform(swizzle, swizzle.begin(), ConvertGreenRed);
        }
    }
    const auto format_info = device->surfaceFormat(FormatType::Optimal, true, format);
    const vk::ImageViewUsageCreateInfo image_view_usage{image.UsageFlags()};
    const vk::ImageViewCreateInfo create_info{
        {},
        image.Handle(),
        vk::ImageViewType{},
        format_info.format,
        vk::ComponentMapping{ComponentSwizzle(swizzle[0]), ComponentSwizzle(swizzle[1]),
                             ComponentSwizzle(swizzle[2]), ComponentSwizzle(swizzle[3])},
        MakeSubresourceRange(aspect_mask, info.range),
        &image_view_usage};
    const auto create = [&](shader::TextureType tex_type, std::optional<u32> num_layers) {
        vk::ImageViewCreateInfo ci{create_info};
        ci.viewType = ImageViewType(tex_type);
        if (num_layers) {
            ci.subresourceRange.layerCount = *num_layers;
        }
        ImageView handle = device->logical().CreateImageView(ci);
        if (device->hasDebuggingToolAttached()) {
            handle.SetObjectNameEXT(texture::Name(*this).c_str());
        }
        image_views[static_cast<size_t>(tex_type)] = std::move(handle);
    };
    switch (info.type) {
        case texture::ImageViewType::e1D:
        case texture::ImageViewType::e1DArray:
            create(shader::TextureType::Color1D, 1);
            create(shader::TextureType::ColorArray1D, std::nullopt);
            render_target = Handle(TextureType::ColorArray1D);
            break;
        case texture::ImageViewType::e2D:
        case texture::ImageViewType::e2DArray:
        case texture::ImageViewType::Rect:
            create(shader::TextureType::Color2D, 1);
            create(shader::TextureType::ColorArray2D, std::nullopt);
            render_target = Handle(shader::TextureType::ColorArray2D);
            break;
        case texture::ImageViewType::e3D:
            create(TextureType::Color3D, std::nullopt);
            render_target = Handle(shader::TextureType::Color3D);
            break;
        case texture::ImageViewType::Cube:
        case texture::ImageViewType::CubeArray:
            create(TextureType::ColorCube, 6);
            create(TextureType::ColorArrayCube, std::nullopt);
            break;
        case texture::ImageViewType::Buffer:
            assert(false);
            break;
    }
}

TextureImageView::TextureImageView(TextureCacheRuntime& runtime, const texture::ImageViewInfo& info,
                                   texture::ImageId image_id_, TextureImage& image,
                                   const common::SlotVector<TextureImage>& slot_imgs)
    : TextureImageView{runtime, info, image_id_, image} {
    slot_images = &slot_imgs;
}

TextureImageView::TextureImageView(TextureCacheRuntime&, const texture::ImageInfo& info,
                                   const texture::ImageViewInfo& view_info)
    : texture::ImageViewBase{info, view_info},
      buffer_size{texture::CalculateGuestSizeInBytes(info)} {}
TextureImageView::TextureImageView(TextureCacheRuntime& runtime,
                                   const texture::NullImageViewParams& params)
    : texture::ImageViewBase{params}, device{&runtime.device} {
    if (device->hasNullDescriptor()) {
        return;
    }

    // Handle fallback for devices without nullDescriptor
    texture::ImageInfo info{};
    info.format = surface::PixelFormat::A8B8G8R8_UNORM;

    null_image = MakeImage(*device, runtime.memory_allocator, info, {});
    image_handle = *null_image;
    for (u32 i = 0; i < shader::NUM_TEXTURE_TYPES; i++) {
        image_views[i] =
            MakeView(vk::Format::eA8B8G8R8UnormPack32, vk::ImageAspectFlagBits::eColor);
    }
}

auto TextureImageView::IsRescaled() const noexcept -> bool {
    if (!slot_images) {
        return false;
    }
    const auto& slots = *slot_images;
    const auto& src_image = slots[image_id];
    return src_image.IsRescaled();
}

auto TextureImageView::MakeView(vk::Format vk_format, vk::ImageAspectFlags aspect_mask)
    -> ImageView {
    return device->logical().CreateImageView(
        vk::ImageViewCreateInfo{{},
                                image_handle,
                                ImageViewType(type),
                                vk_format,
                                vk::ComponentMapping{
                                    vk::ComponentSwizzle::eIdentity,
                                    vk::ComponentSwizzle::eIdentity,
                                    vk::ComponentSwizzle::eIdentity,
                                    vk::ComponentSwizzle::eIdentity,
                                },
                                MakeSubresourceRange(aspect_mask, range)});
}

auto TextureImageView::DepthView() -> vk::ImageView {
    if (!image_handle) {
        return VK_NULL_HANDLE;
    }
    if (depth_view) {
        return *depth_view;
    }
    const auto& info = device->surfaceFormat(FormatType::Optimal, true, format);
    depth_view = MakeView(info.format, vk::ImageAspectFlagBits::eDepth);
    return *depth_view;
}

auto TextureImageView::StencilView() -> vk::ImageView {
    if (!image_handle) {
        return VK_NULL_HANDLE;
    }
    if (stencil_view) {
        return *stencil_view;
    }
    const auto& info = device->surfaceFormat(FormatType::Optimal, true, format);
    stencil_view = MakeView(info.format, vk::ImageAspectFlagBits::eStencil);
    return *stencil_view;
}

auto TextureImageView::ColorView() -> vk::ImageView {
    if (!image_handle) {
        return VK_NULL_HANDLE;
    }
    if (color_view) {
        return *color_view;
    }
    color_view = MakeView(vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
    return *color_view;
}

auto TextureImageView::StorageView(shader::TextureType texture_type,
                                   shader::ImageFormat image_format) -> vk::ImageView {
    if (!image_handle) {
        return VK_NULL_HANDLE;
    }
    if (image_format == shader::ImageFormat::Typeless) {
        return Handle(texture_type);
    }
    const bool is_signed{image_format == shader::ImageFormat::R8_SINT ||
                         image_format == shader::ImageFormat::R16_SINT};
    if (!storage_views) {
        storage_views = std::make_unique<StorageViews>();
    }
    auto& views{is_signed ? storage_views->signeds : storage_views->unsigneds};
    auto& view{views[static_cast<size_t>(texture_type)]};
    if (view) {
        return *view;
    }
    view = MakeView(Format(image_format), vk::ImageAspectFlagBits::eColor);
    return *view;
}

TextureImageView::~TextureImageView() = default;

// TODO 这些需要修改
TextureSampler::TextureSampler(TextureCacheRuntime& runtime, SamplerReduction reduction,
                               int32_t imageMipLevels) {
    const auto& device = runtime.device;
    const bool arbitrary_borders = runtime.device.isExtCustomBorderColorSupported();
    auto border_color = std::array<float, 4>{0, 0, 0, 0};
    vk::ClearColorValue color{std::bit_cast<vk::ClearColorValue>(border_color)};
    const vk::SamplerCustomBorderColorCreateInfoEXT border_ci{
        color,
        vk::Format::eUndefined,
    };
    const void* pnext = nullptr;
    if (arbitrary_borders) {
        pnext = &border_ci;
    }
    const vk::SamplerReductionModeCreateInfoEXT reduction_ci{samplerReduction(reduction), pnext};
    if (runtime.device.isExtSamplerFilterMinmaxSupported()) {
        pnext = &reduction_ci;
    } else if (reduction_ci.reductionMode != vk::SamplerReductionMode::eWeightedAverage) {
        SPDLOG_WARN("VK_EXT_sampler_filter_minmax is required");
    }
    // Some games have samplers with garbage. Sanitize them here.
    const f32 max_anisotropy = std::clamp(device.getMaxAnisotropy(), 1.0f, 16.0f);

    const auto create_sampler = [&](const f32 anisotropy) {
        return device.logical().CreateSampler(vk::SamplerCreateInfo{
            {},
            ::vk::Filter::eLinear,
            ::vk::Filter::eLinear,
            ::vk::SamplerMipmapMode::eLinear,
            ::vk::SamplerAddressMode::eRepeat,
            ::vk::SamplerAddressMode::eRepeat,
            ::vk::SamplerAddressMode::eRepeat,
            0.0f,
            static_cast<VkBool32>(anisotropy > 1.0f ? VK_TRUE : VK_FALSE),
            anisotropy,
            VK_FALSE,
            ::vk::CompareOp::eAlways,
            0.0f,
            static_cast<float>(imageMipLevels),
            arbitrary_borders ? vk::BorderColor::eFloatCustomEXT : ConvertBorderColor(border_color),
            VK_FALSE,
            pnext});
    };

    sampler = create_sampler(max_anisotropy);

    const f32 max_anisotropy_default =
        static_cast<f32>(1U << static_cast<int>(device.getMaxAnisotropy()));
    if (max_anisotropy > max_anisotropy_default) {
        sampler_default_anisotropy = create_sampler(max_anisotropy_default);
    }
}

TextureFramebuffer::TextureFramebuffer(TextureCacheRuntime& runtime,
                                       std::span<TextureImageView*, texture::NUM_RT> color_buffers,
                                       TextureImageView* depth_buffer,
                                       const texture::RenderTargets& key)
    : render_area{vk::Extent2D{key.size.width, key.size.height}} {
    CreateFramebuffer(runtime, color_buffers, depth_buffer, key.is_rescaled);
    if (runtime.device.hasDebuggingToolAttached()) {
        framebuffer.SetObjectNameEXT(texture::Name(key).c_str());
    }
}

TextureFramebuffer::TextureFramebuffer(TextureCacheRuntime& runtime, TextureImageView* color_buffer,
                                       TextureImageView* depth_buffer, vk::Extent2D extent,
                                       bool is_rescaled)
    : render_area{extent} {
    std::array<TextureImageView*, texture::NUM_RT> color_buffers{color_buffer};
    CreateFramebuffer(runtime, color_buffers, depth_buffer, is_rescaled);
}

TextureFramebuffer::~TextureFramebuffer() = default;

void TextureFramebuffer::CreateFramebuffer(
    TextureCacheRuntime& runtime, std::span<TextureImageView*, texture::NUM_RT> color_buffers,
    TextureImageView* depth_buffer, bool is_rescaled_) {
    boost::container::small_vector<vk::ImageView, texture::NUM_RT + 1> attachments;
    RenderPassKey renderpass_key{};
    s32 num_layers = 1;

    is_rescaled = is_rescaled_;
    const auto& resolution = runtime.resolution;

    u32 width = std::numeric_limits<u32>::max();
    u32 height = std::numeric_limits<u32>::max();
    for (size_t index = 0; index < texture::NUM_RT; ++index) {
        const TextureImageView* const color_buffer = color_buffers[index];
        if (!color_buffer) {
            renderpass_key.color_formats[index] = surface::PixelFormat::Invalid;
            continue;
        }
        width = std::min(width, is_rescaled ? resolution.ScaleUp(color_buffer->size.width)
                                            : color_buffer->size.width);
        height = std::min(height, is_rescaled ? resolution.ScaleUp(color_buffer->size.height)
                                              : color_buffer->size.height);
        attachments.push_back(color_buffer->RenderTarget());
        renderpass_key.color_formats[index] = color_buffer->format;
        num_layers = std::max(num_layers, color_buffer->range.extent.layers);
        images[num_images] = color_buffer->ImageHandle();
        image_ranges[num_images] = MakeSubresourceRange(color_buffer);
        rt_map[index] = num_images;
        samples = color_buffer->Samples();
        ++num_images;
    }
    const size_t num_colors = attachments.size();
    if (depth_buffer) {
        width = std::min(width, is_rescaled ? resolution.ScaleUp(depth_buffer->size.width)
                                            : depth_buffer->size.width);
        height = std::min(height, is_rescaled ? resolution.ScaleUp(depth_buffer->size.height)
                                              : depth_buffer->size.height);
        attachments.push_back(depth_buffer->RenderTarget());
        renderpass_key.depth_format = depth_buffer->format;
        num_layers = std::max(num_layers, depth_buffer->range.extent.layers);
        images[num_images] = depth_buffer->ImageHandle();
        const VkImageSubresourceRange subresource_range = MakeSubresourceRange(depth_buffer);
        image_ranges[num_images] = subresource_range;
        samples = depth_buffer->Samples();
        ++num_images;
        has_depth = (subresource_range.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != 0;
        has_stencil = (subresource_range.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0;
    } else {
        renderpass_key.depth_format = surface::PixelFormat::Invalid;
    }
    renderpass_key.samples = samples;

    renderpass = runtime.render_pass_cache.get(renderpass_key);
    render_area.width = std::min(render_area.width, width);
    render_area.height = std::min(render_area.height, height);

    num_color_buffers = static_cast<u32>(num_colors);
    framebuffer = runtime.device.logical().createFramerBuffer(vk::FramebufferCreateInfo{
        {},
        renderpass,
        static_cast<u32>(attachments.size()),
        attachments.data(),
        render_area.width,
        render_area.height,
        static_cast<u32>(std::max(num_layers, 1)),
    });
}

void TextureCacheRuntime::AccelerateImageUpload(
    TextureImage& image, const StagingBufferRef& map,
    std::span<const texture::SwizzleParameters> swizzles) {
    if (IsPixelFormatASTC(image.info.format)) {
        return astc_decoder_pass->Assemble(image, map, swizzles);
    }
    assert(false);
}

void TextureCacheRuntime::TransitionImageLayout(TextureImage& image) {
    if (!image.ExchangeInitialization()) {
        vk::ImageMemoryBarrier barrier{
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            image.Handle(),
            vk::ImageSubresourceRange{
                image.AspectMask(),
                0,
                VK_REMAINING_MIP_LEVELS,
                0,
                VK_REMAINING_ARRAY_LAYERS,
            },
        };
        scheduler.requestOutsideRenderPassOperationContext();
        scheduler.record([barrier](vk::CommandBuffer cmdbuf) {
            cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                   vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, barrier);
        });
    }
}

}  // namespace render::vulkan