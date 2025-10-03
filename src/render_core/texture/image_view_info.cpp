#include "image_view_info.hpp"
namespace render::texture {
namespace {}  // namespace

ImageViewInfo::ImageViewInfo(const ImageInfo& info, ImageViewType type) noexcept
    : type{type},
      format{info.format},
      range{SubresourceRange{.base = {.level = 0, .layer = 0}, .extent = info.resources}} {}

}  // namespace render::texture