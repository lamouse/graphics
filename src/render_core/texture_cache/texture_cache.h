#pragma once
#include "render_core/texture_cache/texture_cache_base.hpp"
#include "render_core/texture_cache/utils.hpp"
#include "render_core/texture/image_view_base.hpp"
#include <cstring>
#include "common/assert.hpp"
namespace render::texture {
using surface::GetFormatType;
using surface::PixelFormat;
using surface::SurfaceType;
using namespace common::literals;

template <class P>
TextureCache<P>::TextureCache(Runtime& runtime_) : runtime{runtime_} {
    // Make sure the first index is reserved for the null resources
    // This way the null resource becomes a compile time constant
    void(slot_images.insert());
    void(slot_image_views.insert(runtime_, NullImageViewParams{}));
    void(slot_samplers.insert(runtime_, SamplerReduction::WeightedAverage, 1.F));
}
template <class P>
void TextureCache<P>::TickFrame() {
    runtime.TickFrame();
    ++frame_tick;
}

template <class P>
auto TextureCache<P>::addTexture(const Extent2D& extent, std::span<unsigned char> data,
                                 int layer_count) -> ImageViewId {
    ImageInfo info;
    info.size.width = extent.width;
    info.size.height = extent.height;
    info.type = render::texture::ImageType::e2D;
    info.num_samples = 1;
    info.resources.layers = layer_count;
    info.resources.levels = 1;
    info.format = PixelFormat::B8G8R8A8_SRGB;
    const ImageId new_image_id = slot_images.insert(runtime, info);
    Image& new_image = slot_images[new_image_id];
    auto staging = runtime.UploadStagingBuffer(data.size());
    std::memcpy(staging.mapped_span.data(), data.data(), data.size());

    std::vector<BufferImageCopy> copys;
    auto single_data_size = data.size() / layer_count;
    for (int i = 0; i < layer_count; i++) {
        BufferImageCopy copy{
            .buffer_offset = i * single_data_size,
            .buffer_size = single_data_size,
            .buffer_row_length = 0,
            .buffer_image_height = 0,
            .image_subresource = {.base_level = 0, .base_layer = i, .num_layers = 1},
            .image_offset = {.x = 0, .y = 0, .z = 0},
            .image_extent = info.size};
        copys.push_back(copy);
    }
    new_image.UploadMemory(staging, copys);

    ImageViewType type = layer_count == 6 ? ImageViewType::Cube : ImageViewType::e2D;
    const ImageViewInfo view_info(info, type);
    const ImageViewId image_view_id =
        slot_image_views.insert(runtime, view_info, new_image_id, new_image);
    return image_view_id;
}
template <class P>
auto TextureCache<P>::addTexture(ktxTexture* ktxTexture) -> ImageViewId {
    auto* tex2 = reinterpret_cast<ktxTexture2*>(ktxTexture);
    auto format = tex2->vkFormat;
    ImageInfo info;
    info.size.width = ktxTexture->baseWidth;
    info.size.height = ktxTexture->baseHeight;
    info.type = render::texture::ImageType::e2D;
    info.num_samples = 1;
    info.resources.layers = static_cast<s32>(ktxTexture->numFaces);
    info.resources.levels = static_cast<s32>(ktxTexture->numLevels);
    info.format = PixelFormat::B8G8R8A8_UNORM;
    const ImageId new_image_id = slot_images.insert(runtime, info);
    Image& new_image = slot_images[new_image_id];
    auto staging = runtime.UploadStagingBuffer(ktxTexture->dataSize);
    std::memcpy(staging.mapped_span.data(), ktxTexture->pData, ktxTexture->dataSize);

    std::vector<BufferImageCopy> copys;
    for (uint32_t face = 0; face < ktxTexture->numFaces; face++) {
        for (uint32_t level = 0; level < ktxTexture->numLevels; ++level) {
            ktx_size_t offset{};
            KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
            // 计算当前 mip 层的尺寸
            uint32_t mipWidth = std::max(1u, ktxTexture->baseWidth >> level);
            uint32_t mipHeight = std::max(1u, ktxTexture->baseHeight >> level);
            BufferImageCopy copy{
                .buffer_offset = offset,
                .buffer_size = 0,
                .buffer_row_length = 0,
                .buffer_image_height = 0,
                .image_subresource = {.base_level = static_cast<s32>(level),
                                      .base_layer = static_cast<int>(face),
                                      .num_layers = 1},
                .image_offset = {.x = 0, .y = 0, .z = 0},
                .image_extent = {.width = mipWidth, .height = mipHeight, .depth = 1}};
            copys.push_back(copy);
        }
    }
    new_image.UploadMemory(staging, copys);

    ImageViewType type = ktxTexture->isCubemap ? ImageViewType::Cube : ImageViewType::e2D;
    const ImageViewInfo view_info(info, type);
    const ImageViewId image_view_id =
        slot_image_views.insert(runtime, view_info, new_image_id, new_image);
    return image_view_id;
}

template <class P>
auto TextureCache<P>::getSampler(SamplerPreset preset) -> typename P::Sampler* {
    auto& samplerId = sampler_presets[static_cast<uint8_t>(preset)];
    if (samplerId) {
        return &slot_samplers[samplerId];
    }
    samplerId = slot_samplers.insert(runtime, preset);
    return &slot_samplers[samplerId];
}

template <class P>
auto TextureCache<P>::getCurrentTextures() -> std::span<ImageView*> {
    sample_texture.clear();
    for (const auto& textureId : used_textures) {
        sample_texture.push_back(&slot_image_views[textureId]);
    }
    used_textures.clear();
    return sample_texture;
}

template <class P>
void TextureCache<P>::setCurrentTextures(const std::span<const ImageViewId>& textures) {
    for (const auto& texture : textures) {
        if (texture) {
            used_textures.push_back(texture);
        }
    }
}

template <class P>
auto TextureCache<P>::createImageAndView(const ImageInfo& info) -> ImageViewId {
    const ImageId new_image_id = slot_images.insert(runtime, info);
    Image& new_image = slot_images[new_image_id];
    runtime.TransitionImageLayout(new_image);
    const ImageViewInfo view_info(info);
    const ImageViewId image_view_id =
        slot_image_views.insert(runtime, view_info, new_image_id, new_image);
    return image_view_id;
}

template <class P>
auto TextureCache<P>::getFramebuffer() -> Framebuffer* {
    return current_frame_buffer;
}

template <class P>
void TextureCache<P>::UpdateRenderTarget(const FramebufferKey& key) {
    if (frameRenderTarget.contains(key)) {
        render_targets = frameRenderTarget.find(key)->second;
        return;
    }
    RenderTargets target;
    ImageInfo info;
    info.size = key.size;
    info.num_samples = 1;
    target.size = {.width = key.size.width, .height = key.size.height};
    std::size_t index{};
    for (auto format : key.color_formats) {
        if (format == surface::PixelFormat::Invalid) {
            break;
        }
        if (surface::GetFormatType(format) != SurfaceType::ColorTexture) {
            // NOLINTNEXTLINE
            ASSERT_MSG(false, "颜色格式错误");
        }
        info.format = format;
        target.color_buffer_ids.at(index) = createImageAndView(info);
    }
    if (surface::GetFormatType(key.depth_format) == SurfaceType::Depth) {
        info.format = key.depth_format;
        target.depth_buffer_id = createImageAndView(info);
    } else {
        // NOLINTNEXTLINE
        ASSERT_MSG(false, "深度格式错误");
    }

    const auto [pair, is_new] = framebuffers.try_emplace(target);
    if (!is_new) {
        current_frame_buffer = &slot_framebuffers[pair->second];
        return;
    }
    std::array<ImageView*, NUM_RT> color_buffers;
    std::ranges::transform(target.color_buffer_ids, color_buffers.begin(),
                           [this](ImageViewId id) { return id ? &slot_image_views[id] : nullptr; });
    ImageView* const depth_buffer =
        target.depth_buffer_id ? &slot_image_views[target.depth_buffer_id] : nullptr;
    pair->second = slot_framebuffers.insert(runtime, color_buffers, depth_buffer, target);
    current_frame_buffer = &slot_framebuffers[pair->second];
    render_targets = target;
    frameRenderTarget[key] = target;
}

template <class P>
auto TextureCache<P>::TryFindFramebufferImageView() -> std::pair<typename P::ImageView*, bool> {
    if (slot_image_views.size() <= 1) {
        return std::make_pair(nullptr, false);
    }

    return std::make_pair(&slot_image_views[render_targets.color_buffer_ids.at(0)], false);
}

}  // namespace render::texture