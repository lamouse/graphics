module;
#include <cstddef>
#include <boost/container/small_vector.hpp>
export module render.texture_cache.utils;
import render.surface.format;
import common.types;
import render.texture;

export namespace render::texture::utils {
inline auto CalculateGuestSizeInBytes(const texture::ImageInfo& info) -> size_t {
    auto size = static_cast<size_t>(info.size.width) * static_cast<size_t>(info.size.height) *
                static_cast<size_t>(info.size.depth);

    size *= surface::BytesPerBlock(info.format);
    return size;
}

[[nodiscard]] auto MakeShrinkImageCopies(const ImageInfo& dst, const ImageInfo& src,
                                         SubresourceBase base, u32 up_scale = 1, u32 down_shift = 0)
    -> boost::container::small_vector<ImageCopy, 16>;

[[nodiscard]] auto MipSize(Extent3D size, u32 level) -> Extent3D;
}  // namespace render::texture::utils