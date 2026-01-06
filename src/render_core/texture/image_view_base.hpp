#pragma once
#include "render_core/surface.hpp"
#include "render_core/texture/image_view_info.hpp"
#include "render_core/texture/types.hpp"

namespace render::texture {
struct ImageViewInfo;
struct ImageInfo;

struct NullImageViewParams {};

struct ImageViewBase {
        explicit ImageViewBase(const ImageViewInfo& info, const ImageInfo& image_info,
                               ImageId image_id);
        explicit ImageViewBase(const NullImageViewParams&);

        [[nodiscard]] auto SupportsAnisotropy() const noexcept -> bool;

        ImageId image_id{};
        surface::PixelFormat format{};
        ImageViewType type{};
        SubresourceRange range;
        Extent3D size{.width = 0, .height = 0, .depth = 0};
};
}  // namespace render::texture