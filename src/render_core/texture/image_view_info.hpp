#pragma once

#include "render_core/surface.hpp"
#include "render_core/texture/image_info.hpp"
#include "render_core/texture/types.hpp"

namespace render::texture {

struct ImageViewInfo {
        explicit ImageViewInfo() noexcept = default;
        explicit ImageViewInfo(const ImageInfo& info,
                               ImageViewType type = ImageViewType::e2D) noexcept;
        explicit ImageViewInfo(ImageViewType type, surface::PixelFormat format,
                               SubresourceRange range = {}) noexcept
            : type(type), format(format), range(range) {}

        auto operator<=>(const ImageViewInfo&) const noexcept = default;

        ImageViewType type{};
        surface::PixelFormat format{};
        SubresourceRange range;
};
}  // namespace render::texture