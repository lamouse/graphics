#include <algorithm>
#include "surface.hpp"
#include "image_info.hpp"
#include "image_view_base.hpp"
#include "image_view_info.hpp"
#include "types.hpp"
namespace render::texture {
ImageViewBase::ImageViewBase(const ImageViewInfo& info, const ImageInfo& image_info,
                             ImageId image_id_)
    : image_id{image_id_},
      format{info.format},
      type{info.type},
      range{info.range},
      size{
          .width = image_info.size.width,
          .height = image_info.size.height,
          .depth = image_info.size.depth,
      } {}

ImageViewBase::ImageViewBase(const NullImageViewParams& /*unused*/) : image_id{NULL_IMAGE_ID} {}

auto ImageViewBase::SupportsAnisotropy() const noexcept -> bool {
    const bool has_mips = range.extent.levels > 1;
    const bool is_2d = type == ImageViewType::e2D || type == ImageViewType::e2DArray;
    if (!has_mips || !is_2d) {
        return false;
    }

    using namespace surface;
    switch (format) {
        case PixelFormat::R8_UNORM:
        case PixelFormat::R8_SNORM:
        case PixelFormat::R8_SINT:
        case PixelFormat::R8_UINT:
        case PixelFormat::BC4_UNORM:
        case PixelFormat::BC4_SNORM:
        case PixelFormat::BC5_UNORM:
        case PixelFormat::BC5_SNORM:
        case PixelFormat::R32G32_FLOAT:
        case PixelFormat::R32G32_SINT:
        case PixelFormat::R32_FLOAT:
        case PixelFormat::R16_FLOAT:
        case PixelFormat::R16_UNORM:
        case PixelFormat::R16_SNORM:
        case PixelFormat::R16_UINT:
        case PixelFormat::R16_SINT:
        case PixelFormat::R16G16_UNORM:
        case PixelFormat::R16G16_FLOAT:
        case PixelFormat::R16G16_UINT:
        case PixelFormat::R16G16_SINT:
        case PixelFormat::R16G16_SNORM:
        case PixelFormat::R8G8_UNORM:
        case PixelFormat::R8G8_SNORM:
        case PixelFormat::R8G8_SINT:
        case PixelFormat::R8G8_UINT:
        case PixelFormat::R32G32_UINT:
        case PixelFormat::R32_UINT:
        case PixelFormat::R32_SINT:
        case PixelFormat::G4R4_UNORM:
        // Depth formats
        case PixelFormat::D32_FLOAT:
        case PixelFormat::D16_UNORM:
        case PixelFormat::X8_D24_UNORM:
        // Stencil formats
        case PixelFormat::S8_UINT:
        // DepthStencil formats
        case PixelFormat::D24_UNORM_S8_UINT:
        case PixelFormat::S8_UINT_D24_UNORM:
        case PixelFormat::D32_FLOAT_S8_UINT:
            return false;
        default:
            return true;
    }
}
}  // namespace render::texture