#include "image_view_info.hpp"
namespace render::texture {
namespace {
constexpr u8 RENDER_TARGET_SWIZZLE = std::numeric_limits<u8>::max();
[[nodiscard]] auto CastSwizzle(SwizzleSource source) -> u8 {
    const u8 casted = static_cast<u8>(source);
    assert(static_cast<SwizzleSource>(casted) == source);
    return casted;
}
}  // namespace

ImageViewInfo::ImageViewInfo(ImageViewType type_, surface::PixelFormat format_,
                             SubresourceRange range_) noexcept
    : type{type_},
      format{format_},
      range{range_},
      x_source{RENDER_TARGET_SWIZZLE},
      y_source{RENDER_TARGET_SWIZZLE},
      z_source{RENDER_TARGET_SWIZZLE},
      w_source{RENDER_TARGET_SWIZZLE} {}
auto ImageViewInfo::IsRenderTarget() const noexcept -> bool {
    return x_source == RENDER_TARGET_SWIZZLE && y_source == RENDER_TARGET_SWIZZLE &&
           z_source == RENDER_TARGET_SWIZZLE && w_source == RENDER_TARGET_SWIZZLE;
}

ImageViewInfo::ImageViewInfo(const ImageInfo& info) noexcept
    : type{info.type == ImageType::Buffer ? ImageViewType::Buffer : ImageViewType::e2D},
      format{info.format},
      range{SubresourceRange{.base = {0, 0}, .extent = info.resources}},
      x_source{CastSwizzle(SwizzleSource::R)},
      y_source{CastSwizzle(SwizzleSource::G)},
      z_source{CastSwizzle(SwizzleSource::B)},
      w_source{CastSwizzle(SwizzleSource::A)} {}

}  // namespace render::texture