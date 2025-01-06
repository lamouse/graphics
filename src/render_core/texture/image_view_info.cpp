#include "image_view_info.hpp"
namespace render::texture {
namespace {}

ImageViewInfo::ImageViewInfo(ImageViewType type_, surface::PixelFormat format_,
                             SubresourceRange range_) noexcept
    : type{type_}, format{format_}, range{range_} {}

}  // namespace render::texture