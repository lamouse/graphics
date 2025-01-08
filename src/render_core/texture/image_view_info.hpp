#pragma once

#include <type_traits>
#include "surface.hpp"
#include "types.hpp"
#include "texture.hpp"

namespace render::texture {

struct ImageViewInfo {
        explicit ImageViewInfo() noexcept = default;
        explicit ImageViewInfo(ImageViewType type, surface::PixelFormat format,
                               SubresourceRange range = {}) noexcept;

        auto operator<=>(const ImageViewInfo&) const noexcept = default;

        [[nodiscard]] auto IsRenderTarget() const noexcept -> bool;
        [[nodiscard]] auto Swizzle() const noexcept -> std::array<SwizzleSource, 4> {
            return std::array{
                static_cast<SwizzleSource>(x_source),
                static_cast<SwizzleSource>(y_source),
                static_cast<SwizzleSource>(z_source),
                static_cast<SwizzleSource>(w_source),
            };
        }

        ImageViewType type{};
        surface::PixelFormat format{};
        SubresourceRange range;
        u8 x_source = static_cast<u8>(SwizzleSource::R);
        u8 y_source = static_cast<u8>(SwizzleSource::G);
        u8 z_source = static_cast<u8>(SwizzleSource::B);
        u8 w_source = static_cast<u8>(SwizzleSource::A);
};
static_assert(std::has_unique_object_representations_v<ImageViewInfo>);
}  // namespace render::texture