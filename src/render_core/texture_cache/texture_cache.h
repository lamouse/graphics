#pragma once
#include "render_core/texture_cache/texture_cache_base.hpp"
#include "render_core/texture/image_base.hpp"
#include "render_core/texture_cache/utils.hpp"
#include "render_core/texture/samples_helper.h"
#include "render_core/texture/image_view_base.hpp"
#include <algorithm>
#include <cstring>
namespace render::texture {
using surface::GetFormatType;
using surface::PixelFormat;
using surface::SurfaceType;
using namespace common::literals;

template <class P>
TextureCache<P>::TextureCache(Runtime& runtime_) : runtime{runtime_} {}

template <class P>
void TextureCache<P>::WriteMemory(void* data, size_t size) {}

template <class P>
auto TextureCache<P>::InsertImage(const ImageInfo& info) -> ImageId {
    const ImageId image_id = JoinImages(info);
    const Image& image = slot_images[image_id];
    return image_id;
}

template <class P>
auto TextureCache<P>::JoinImages(const ImageInfo& info) -> ImageId {
    ImageInfo new_info = info;
    const size_t size_bytes = utils::CalculateGuestSizeInBytes(new_info);
    join_overlap_ids.clear();
    join_overlaps_found.clear();
    join_left_aliased_ids.clear();
    join_right_aliased_ids.clear();
    join_ignore_textures.clear();
    join_bad_overlap_ids.clear();
    join_copies_to_do.clear();
    join_alias_indices.clear();

    const ImageId new_image_id = slot_images.insert(runtime, new_info);
    Image& new_image = slot_images[new_image_id];

    auto staging = runtime.UploadStagingBuffer(size_bytes);

    std::memcpy(staging.mapped_span.data(), info.data, size_bytes);
    BufferImageCopy copys[1];
    copys[0].image_offset = {0, 0, 0};
    copys[0].image_extent = new_info.size;
    copys[0].buffer_offset = 0;
    copys[0].buffer_size = size_bytes;
    copys[0].buffer_row_length = 0;
    copys[0].buffer_image_height = 0;
    new_image.UploadMemory(staging, copys);
    AddImageAlias(new_image, new_image, new_image_id, new_image_id);
    return new_image_id;
}

template <class P>
void TextureCache<P>::CopyImage(ImageId dst_id, ImageId src_id, std::vector<ImageCopy> copies) {
    Image& dst = slot_images[dst_id];
    Image& src = slot_images[src_id];
    const auto dst_format_type = GetFormatType(dst.info.format);
    const auto src_format_type = GetFormatType(src.info.format);
    if (src_format_type == dst_format_type) {
        if constexpr (HAS_EMULATED_COPIES) {
            if (!runtime.CanImageBeCopied(dst, src)) {
                return runtime.EmulateCopyImage(dst, src, copies);
            }
        }
        return runtime.CopyImage(dst, src, copies);
    }
    if (runtime.ShouldReinterpret(dst, src)) {
        return runtime.ReinterpretImage(dst, src, copies);
    }
    for (const ImageCopy& copy : copies) {
        const SubresourceBase dst_base{
            .level = copy.dst_subresource.base_level,
            .layer = copy.dst_subresource.base_layer,
        };
        const SubresourceBase src_base{
            .level = copy.src_subresource.base_level,
            .layer = copy.src_subresource.base_layer,
        };
        const SubresourceExtent dst_extent{.levels = 1, .layers = 1};
        const SubresourceExtent src_extent{.levels = 1, .layers = 1};
        const SubresourceRange dst_range{.base = dst_base, .extent = dst_extent};
        const SubresourceRange src_range{.base = src_base, .extent = src_extent};
        PixelFormat dst_format = dst.info.format;
        if (GetFormatType(src.info.format) == SurfaceType::DepthStencil &&
            GetFormatType(dst_format) == SurfaceType::ColorTexture &&
            BytesPerBlock(dst_format) == 4) {
            dst_format = PixelFormat::A8B8G8R8_UNORM;
        }
        const ImageViewInfo dst_view_info(ImageViewType::e2D, dst_format, dst_range);
        const ImageViewInfo src_view_info(ImageViewType::e2D, src.info.format, src_range);
        const auto [dst_framebuffer_id, dst_view_id] = RenderTargetFromImage(dst_id, dst_view_info);
        Framebuffer* const dst_framebuffer = &slot_framebuffers[dst_framebuffer_id];
        const ImageViewId src_view_id = FindOrEmplaceImageView(src_id, src_view_info);
        ImageView& dst_view = slot_image_views[dst_view_id];
        ImageView& src_view = slot_image_views[src_view_id];
        [[maybe_unused]] const Extent3D expected_size{
            .width = std::min(dst_view.size.width, src_view.size.width),
            .height = std::min(dst_view.size.height, src_view.size.height),
            .depth = std::min(dst_view.size.depth, src_view.size.depth),
        };

        runtime.ConvertImage(dst_framebuffer, dst_view, src_view);
    }
}

template <class P>
auto TextureCache<P>::FindOrEmplaceImageView(ImageId image_id, const ImageViewInfo& info)
    -> ImageViewId {
    Image& image = slot_images[image_id];
    if (const ImageViewId image_view_id = image.FindView(info); image_view_id) {
        return image_view_id;
    }
    const ImageViewId image_view_id =
        slot_image_views.insert(runtime, info, image_id, image, slot_images);
    image.InsertView(info, image_view_id);
    return image_view_id;
}

template <class P>
auto TextureCache<P>::RenderTargetFromImage(ImageId image_id, const ImageViewInfo& view_info)
    -> std::pair<FramebufferId, ImageViewId> {
    const ImageViewId view_id = FindOrEmplaceImageView(image_id, view_info);
    const ImageBase& image = slot_images[image_id];
    const bool is_rescaled = True(image.flags & ImageFlagBits::Rescaled);
    const bool is_color = GetFormatType(image.info.format) == SurfaceType::ColorTexture;
    const ImageViewId color_view_id = is_color ? view_id : ImageViewId{};
    const ImageViewId depth_view_id = is_color ? ImageViewId{} : view_id;
    Extent3D extent = utils::MipSize(image.info.size, view_info.range.base.level);
    const u32 num_samples = image.info.num_samples;
    const auto [samples_x, samples_y] = SamplesLog2(num_samples);
    render_targets = RenderTargets{
        .color_buffer_ids = {color_view_id},
        .depth_buffer_id = depth_view_id,
        .size = {extent.width >> samples_x, extent.height >> samples_y},
        .is_rescaled = is_rescaled,
    };
    const FramebufferId framebuffer_id = GetFramebufferId(render_targets);

    return {framebuffer_id, view_id};
}

template <class P>
auto TextureCache<P>::GetFramebufferId(const RenderTargets& key) -> FramebufferId {
    const auto [pair, is_new] = framebuffers.try_emplace(key);
    FramebufferId& framebuffer_id = pair->second;
    if (!is_new) {
        return framebuffer_id;
    }
    std::array<ImageView*, NUM_RT> color_buffers;
    std::ranges::transform(key.color_buffer_ids, color_buffers.begin(),
                           [this](ImageViewId id) { return id ? &slot_image_views[id] : nullptr; });
    ImageView* const depth_buffer =
        key.depth_buffer_id ? &slot_image_views[key.depth_buffer_id] : nullptr;
    framebuffer_id = slot_framebuffers.insert(runtime, color_buffers, depth_buffer, key);
    return framebuffer_id;
}

template <class P>
auto TextureCache<P>::GetFramebuffer() -> typename P::Framebuffer* {
    return &slot_framebuffers[GetFramebufferId(render_targets)];
}

template <class P>
auto TextureCache<P>::FindSampler(u32 index) -> SamplerId {
    if (index > slot_samplers.size()) {
        SamplerId id = slot_samplers.insert(runtime, SamplerReduction::Max, 1);
    }

    return slot_samplers[index];
}

template <class P>
auto TextureCache<P>::GetGraphicsSamplerId(u32 index) -> SamplerId {
    return FindSampler(index);
}

template <class P>
auto TextureCache<P>::GetGraphicsSampler(u32 index) -> typename P::Sampler* {
    return &slot_samplers[GetGraphicsSamplerId(index)];
}

template <class P>
auto TextureCache<P>::GetComputeSampler(u32 index) -> typename P::Sampler* {
    return &slot_samplers[GetComputeSamplerId(index)];
}

template <class P>
auto TextureCache<P>::GetSampler(SamplerId id) const noexcept -> const typename P::Sampler& {
    return slot_samplers[id];
}

template <class P>
auto TextureCache<P>::GetSampler(SamplerId id) noexcept -> typename P::Sampler& {
    return slot_samplers[id];
}

template <class P>
auto TextureCache<P>::FindImageView(const ImageInfo& info) -> ImageViewId {
    return CreateImageView(info);
}

template <class P>
auto TextureCache<P>::CreateImageView(const ImageInfo& info) -> ImageViewId {
    if (info.type == ImageType::Buffer) {
        const ImageViewInfo view_info(info);
        return slot_image_views.insert(runtime, info, view_info);
    }
    const ImageId image_id = FindOrInsertImage(info);
    if (!image_id) {
        return NULL_IMAGE_VIEW_ID;
    }
    ImageBase& image = slot_images[image_id];
    const ImageViewInfo view_info(info);
    const ImageViewId image_view_id = FindOrEmplaceImageView(image_id, view_info);
    ImageViewBase& image_view = slot_image_views[image_view_id];
    image_view.flags |= ImageViewFlagBits::Strong;
    image.flags |= ImageFlagBits::Strong;
    RenderTargetFromImage(image_id, view_info);
    return image_view_id;
}

template <class P>
auto TextureCache<P>::FindOrInsertImage(const ImageInfo& info) -> ImageId {
    if (const ImageId image_id = FindImage(info); image_id) {
        return image_id;
    }
    return InsertImage(info);
}

template <class P>
auto TextureCache<P>::FindImage(const ImageInfo& info) -> ImageId {
    return ImageId{};
}

template <class P>
auto TextureCache<P>::CreateSampler(ImageViewId id) -> SamplerId {
    const ImageViewBase& image_view = slot_image_views[id];
    return slot_samplers.insert(runtime, SamplerReduction::WeightedAverage,
                                image_view.range.extent.levels);
}

template <class P>
auto TextureCache<P>::GetImageView(ImageViewId id) const noexcept -> const typename P::ImageView& {
    return slot_image_views[id];
}

template <class P>
auto TextureCache<P>::GetImageView(ImageViewId id) noexcept -> typename P::ImageView& {
    return slot_image_views[id];
}

}  // namespace render::texture