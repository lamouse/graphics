module;


#include <ktx.h>
#include <span>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <array>
#include <vector>
#include <stdexcept>
#include <cstring>
export module render.texture.texture_cache;
import render.texture.render_targets;
import render.texture.types;
import render.surface.format;
import render.texture_enum;
import render.texture;
import render;
import common;
export namespace render::texture {
// 定义 key
struct FramebufferKey {
        std::array<render::surface::PixelFormat, 8> color_formats{
            render::surface::PixelFormat::Invalid};
        render::surface::PixelFormat depth_format = surface::PixelFormat::Invalid;
        render::texture::Extent3D size{};
        auto operator==(const FramebufferKey&) const -> bool = default;
};
}  // namespace render::texture

export namespace std {
template <>
struct hash<render::texture::FramebufferKey> {
        auto operator()(const render::texture::FramebufferKey& key) const noexcept -> size_t {
            size_t h = 0;

            // Hash color_formats
            for (const auto& fmt : key.color_formats) {
                if (fmt == render::surface::PixelFormat::Invalid) {
                    break;
                }  // 假设 Invalid 表示结束
                h ^= std::hash<int>{}(static_cast<int>(fmt)) + 0x9e3779b9 + (h << 6U) + (h >> 2U);
            }

            // Hash depth format
            h ^= std::hash<int>{}(static_cast<int>(key.depth_format)) + 0x9e3779b9 + (h << 6U) +
                 (h >> 2U);

            // Hash size
            h ^= std::hash<render::texture::Extent3D>{}(key.size) + 0x9e3779b9 + (h << 6U) +
                 (h >> 2U);

            return h;
        }
};
}  // namespace std
export namespace render::texture {
using namespace common::literals;
struct ImageViewInOut {
        u32 index{};
        bool blacklist{};
        ImageViewId id{};
};

template <class P>
class TextureCache {
        /// Enables debugging features to the texture cache
        static constexpr bool ENABLE_VALIDATION = P::ENABLE_VALIDATION;
        /// Implement blits as copies between framebuffers
        static constexpr bool FRAMEBUFFER_BLITS = P::FRAMEBUFFER_BLITS;
        /// True when some copies have to be emulated
        static constexpr bool HAS_EMULATED_COPIES = P::HAS_EMULATED_COPIES;
        /// True when the API can provide info about the memory of the device.
        static constexpr bool HAS_DEVICE_MEMORY_INFO = P::HAS_DEVICE_MEMORY_INFO;
        /// True when the API can do asynchronous texture downloads.
        static constexpr bool IMPLEMENTS_ASYNC_DOWNLOADS = P::IMPLEMENTS_ASYNC_DOWNLOADS;

        static constexpr size_t UNSET_CHANNEL{std::numeric_limits<size_t>::max()};

        static constexpr s64 TARGET_THRESHOLD = 4_GiB;
        static constexpr s64 DEFAULT_EXPECTED_MEMORY = 1_GiB + 125_MiB;
        static constexpr s64 DEFAULT_CRITICAL_MEMORY = 1_GiB + 625_MiB;
        static constexpr size_t GC_EMERGENCY_COUNTS = 2;

        using Runtime = typename P::Runtime;
        using Image = typename P::Image;
        using ImageView = typename P::ImageView;
        using Sampler = typename P::Sampler;
        using Framebuffer = typename P::Framebuffer;
        using AsyncBuffer = typename P::AsyncBuffer;
        using BufferType = typename P::BufferType;

    public:
        explicit TextureCache(Runtime& runtime);
        TextureCache(const TextureCache&) = delete;
        TextureCache(TextureCache&&) noexcept = delete;
        auto operator=(const TextureCache&) -> TextureCache& = delete;
        auto operator=(TextureCache&&) noexcept -> TextureCache& = delete;
        ~TextureCache() = default;
        /// Notify the cache that a new frame has been queued
        void TickFrame();

        // 添加一个纹理
        auto addTexture(const Extent2D& extent, std::span<unsigned char> data, int layer_count = 1)
            -> ImageViewId;

        auto addTexture(ktxTexture* ktxTexture) -> ImageViewId;
        auto getSampler(SamplerPreset preset) -> typename P::Sampler*;

        void setCurrentTextures(const std::span<const ImageViewId>& textures);
        auto getCurrentTextures() -> std::span<ImageView*>;
        void UpdateRenderTarget(const FramebufferKey& key);
        auto TryFindFramebufferImageView() -> std::pair<typename P::ImageView*, bool>;
        auto getFramebuffer() -> Framebuffer*;
        std::recursive_mutex mutex;

    private:
        auto createImageAndView(const ImageInfo& info) -> ImageViewId;
        Runtime& runtime;
        common::SlotVector<Image> slot_images;
        common::SlotVector<ImageView> slot_image_views;
        common::SlotVector<Sampler> slot_samplers;
        common::SlotVector<Framebuffer> slot_framebuffers;
        ImageViewId currentTextureId;
        std::vector<ImageView*> sample_texture;
        std::array<SamplerId, static_cast<uint8_t>(SamplerPreset::MAX_PRESET)> sampler_presets;
        std::vector<ImageViewId> used_textures;
        RenderTargets render_targets;
        Framebuffer* current_frame_buffer{};
        std::unordered_map<RenderTargets, FramebufferId> framebuffers;
        std::unordered_map<FramebufferKey, RenderTargets> frameRenderTarget;
        u64 frame_tick = 0;
};

}  // namespace render::texture


export namespace render::texture {
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
    void(slot_samplers.insert(runtime_, SamplerReduction::WeightedAverage, 1));
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
            if (result != KTX_SUCCESS) {
                throw std::runtime_error("ktxTexture_GetImageOffset");
            }
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
            throw std::runtime_error("颜色格式错误");
        }
        info.format = format;
        target.color_buffer_ids.at(index) = createImageAndView(info);
    }
    if (surface::GetFormatType(key.depth_format) == SurfaceType::Depth) {
        info.format = key.depth_format;
        target.depth_buffer_id = createImageAndView(info);
    } else {
        // NOLINTNEXTLINE
        throw std::runtime_error("深度格式错误");
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