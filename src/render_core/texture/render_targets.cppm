module;
#include <algorithm>
#include <span>
#include <array>

#include "common/bit_cast.h"
export module render.texture.render_targets;
import render.texture.types;
import common;

export namespace render::texture {

/// Framebuffer properties used to lookup a framebuffer
struct RenderTargets {
    constexpr auto operator==(const RenderTargets&) const noexcept -> bool = default;

    [[nodiscard]] constexpr auto Contains(std::span<const ImageViewId> elements) const noexcept
        -> bool {
        const auto contains = [elements](ImageViewId item) {
            return std::ranges::find(elements, item) != elements.end();
        };
        return std::ranges::any_of(color_buffer_ids, contains) || contains(depth_buffer_id);
    }

    std::array<ImageViewId, NUM_RT> color_buffer_ids{};
    ImageViewId depth_buffer_id{};
    std::array<u8, NUM_RT> draw_buffers{};
    Extent2D size{};
};

}  // namespace render::texture

export namespace std {

template <>
struct hash<render::texture::RenderTargets> {
    auto operator()(const render::texture::RenderTargets& rt) const noexcept -> size_t {
        using render::texture::ImageViewId;
        size_t value = std::hash<ImageViewId>{}(rt.depth_buffer_id);
        for (const ImageViewId color_buffer_id : rt.color_buffer_ids) {
            if (color_buffer_id) {
                value ^= std::hash<ImageViewId>{}(color_buffer_id);
            }
        }
        value ^= common::BitCast<u64>(rt.draw_buffers);
        value ^= common::BitCast<u64>(rt.size);
        return value;
    }
};

}  // namespace std