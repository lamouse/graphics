#pragma once
#include "render_core/texture_cache/texture_cache_base.hpp"
#include "render_core/texture/image_base.hpp"
#include "render_core/texture_cache/utils.hpp"
#include "render_core/texture/samples_helper.h"
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
auto TextureCache<P>::InsertImage(const ImageInfo& info, RelaxedOptions options) -> ImageId {
    const ImageId image_id = JoinImages(info);
    const Image& image = slot_images[image_id];
    // Using "image.gpu_addr" instead of "gpu_addr" is important because it might be different
    const auto [it, is_new] = image_allocs_table.try_emplace(image.gpu_addr);
    if (is_new) {
        it->second = slot_image_allocs.insert();
    }
    slot_image_allocs[it->second].images.push_back(image_id);
    return image_id;
}

template <class P>
auto TextureCache<P>::JoinImages(const ImageInfo& info) -> ImageId {
    ImageInfo new_info = info;
    const size_t size_bytes = utils::CalculateGuestSizeInBytes(new_info);
    const bool broken_views = runtime.HasBrokenTextureViewFormats();
    const bool native_bgr = runtime.HasNativeBgr();
    join_overlap_ids.clear();
    join_overlaps_found.clear();
    join_left_aliased_ids.clear();
    join_right_aliased_ids.clear();
    join_ignore_textures.clear();
    join_bad_overlap_ids.clear();
    join_copies_to_do.clear();
    join_alias_indices.clear();

    const ImageId new_image_id = slot_images.insert(runtime, new_info, 0, 0);
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

    // image.UploadMemory(staging, {});

    // std::ranges::sort(join_copies_to_do, [this](const JoinCopy& lhs, const JoinCopy& rhs) {
    //     const ImageBase& lhs_image = slot_images[lhs.id];
    //     const ImageBase& rhs_image = slot_images[rhs.id];
    //     return lhs_image.modification_tick < rhs_image.modification_tick;
    // });

    // ImageBase& new_image_base = new_image;
    // for (const ImageId aliased_id : join_right_aliased_ids) {
    //     ImageBase& aliased = slot_images[aliased_id];
    //     size_t alias_index = new_image_base.aliased_images.size();
    //     if (!AddImageAlias(new_image_base, aliased, new_image_id, aliased_id)) {
    //         continue;
    //     }
    //     join_alias_indices.emplace(aliased_id, alias_index);
    //     new_image.flags |= ImageFlagBits::Alias;
    // }
    // for (const ImageId aliased_id : join_left_aliased_ids) {
    //     ImageBase& aliased = slot_images[aliased_id];
    //     size_t alias_index = new_image_base.aliased_images.size();
    //     if (!AddImageAlias(aliased, new_image_base, aliased_id, new_image_id)) {
    //         continue;
    //     }
    //     join_alias_indices.emplace(aliased_id, alias_index);
    //     new_image.flags |= ImageFlagBits::Alias;
    // }
    // for (const ImageId aliased_id : join_bad_overlap_ids) {
    //     ImageBase& aliased = slot_images[aliased_id];
    //     aliased.overlapping_images.push_back(new_image_id);
    //     new_image.overlapping_images.push_back(aliased_id);
    //     if (aliased.info.resources.levels == 1 && aliased.info.block.depth == 0 &&
    //         aliased.overlapping_images.size() > 1) {
    //         aliased.flags |= ImageFlagBits::BadOverlap;
    //     }
    //     if (new_image.info.resources.levels == 1 && new_image.info.block.depth == 0 &&
    //         new_image.overlapping_images.size() > 1) {
    //         new_image.flags |= ImageFlagBits::BadOverlap;
    //     }
    // }

    // for (const auto& copy_object : join_copies_to_do) {
    //     Image& overlap = slot_images[copy_object.id];
    //     if (copy_object.is_alias) {
    //         if (!overlap.IsSafeDownload()) {
    //             continue;
    //         }
    //         const auto alias_pointer = join_alias_indices.find(copy_object.id);
    //         if (alias_pointer == join_alias_indices.end()) {
    //             continue;
    //         }
    //         const AliasedImage& aliased = new_image.aliased_images[alias_pointer->second];
    //         CopyImage(new_image_id, aliased.id, aliased.copies);
    //         new_image.modification_tick = overlap.modification_tick;
    //         continue;
    //     }
    //     if (True(overlap.flags & ImageFlagBits::GpuModified)) {
    //         new_image.flags |= ImageFlagBits::GpuModified;
    //         const SubresourceBase base = new_image.TryFindBase(overlap.gpu_addr).value();

    //         auto copies = utils::MakeShrinkImageCopies(new_info, overlap.info, base, 1, 1);
    //         if (overlap.info.num_samples != new_image.info.num_samples) {
    //             runtime.CopyImageMSAA(new_image, overlap, std::move(copies));
    //         } else {
    //             runtime.CopyImage(new_image, overlap, std::move(copies));
    //         }
    //         new_image.modification_tick = overlap.modification_tick;
    //     }
    //     if (True(overlap.flags & ImageFlagBits::Tracked)) {
    //     }
    // }

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
    const FramebufferId framebuffer_id = GetFramebufferId(RenderTargets{
        .color_buffer_ids = {color_view_id},
        .depth_buffer_id = depth_view_id,
        .size = {extent.width >> samples_x, extent.height >> samples_y},
        .is_rescaled = is_rescaled,
    });
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

}  // namespace render::texture