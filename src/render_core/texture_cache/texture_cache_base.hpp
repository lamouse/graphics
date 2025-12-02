#pragma once

#include "common/scratch_buffer.h"
#include "common/slot_vector.hpp"
#include "common/thread_worker.hpp"
#include "common/literals.hpp"
#include "render_core/texture.hpp"

#include <boost/container/small_vector.hpp>
#include "render_core/texture/types.hpp"
#include "render_core/texture/image_info.hpp"
#include "render_core/texture/render_targets.h"
#include <unordered_set>
#include "common/common_funcs.hpp"
#include <ktx.h>
namespace render::texture {
// 定义 key
struct FramebufferKey {
        std::array<render::surface::PixelFormat, 8> color_formats{
            render::surface::PixelFormat::Invalid};
        render::surface::PixelFormat depth_format = surface::PixelFormat::Invalid;
        render::texture::Extent3D size{};
        auto operator==(const FramebufferKey&) const -> bool = default;
};
}  // namespace render::texture

namespace std {
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
namespace render::texture {
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
        explicit TextureCache(Runtime&);
        CLASS_NON_COPYABLE(TextureCache);
        CLASS_NON_MOVEABLE(TextureCache);
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