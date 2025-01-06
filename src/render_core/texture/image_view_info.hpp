#pragma once

#include <type_traits>
#include "surface.hpp"
#include "types.hpp"

namespace render::texture {
struct ImageViewInfo {
        explicit ImageViewInfo() noexcept = default;
        explicit ImageViewInfo(ImageViewType type, surface::PixelFormat format,
                               SubresourceRange range = {}) noexcept;

        auto operator<=>(const ImageViewInfo&) const noexcept = default;

        [[nodiscard]] bool IsRenderTarget() const noexcept;

        ImageViewType type{};
        surface::PixelFormat format{};
        SubresourceRange range;
};
static_assert(std::has_unique_object_representations_v<ImageViewInfo>);
}  // namespace render::texture