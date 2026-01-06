module;
#include <boost/container/small_vector.hpp>
module render.texture_cache.utils;
import render.texture.sample.helper;
namespace render::texture::utils {
namespace {
[[nodiscard]] constexpr auto AdjustMipSize(u32 size, u32 level) -> u32 {
    return std::max<u32>(size >> level, 1);
}

[[nodiscard]] constexpr auto AdjustMipSize(Extent3D size, s32 level) -> Extent3D {
    return Extent3D{
        .width = AdjustMipSize(size.width, level),
        .height = AdjustMipSize(size.height, level),
        .depth = AdjustMipSize(size.depth, level),
    };
}

[[nodiscard]] auto AdjustSamplesSize(Extent3D size, s32 num_samples) -> Extent3D {
    const auto [samples_x, samples_y] = SamplesLog2(num_samples);
    return Extent3D{
        .width = size.width >> samples_x,
        .height = size.height >> samples_y,
        .depth = size.depth,
    };
}
}  // namespace

auto MakeShrinkImageCopies(const ImageInfo& dst, const ImageInfo& src, SubresourceBase base,
                           u32 up_scale, u32 down_shift)
    -> boost::container::small_vector<ImageCopy, 16> {
    assert(dst.resources.levels >= src.resources.levels);

    const bool is_dst_3d = dst.type == ImageType::e3D;
    if (is_dst_3d) {
        assert(src.type == ImageType::e3D);
        assert(src.resources.levels == 1);
    }
    const bool both_2d{src.type == ImageType::e2D && dst.type == ImageType::e2D};
    boost::container::small_vector<ImageCopy, 16> copies;
    copies.reserve(src.resources.levels);
    for (s32 level = 0; level < src.resources.levels; ++level) {
        ImageCopy& copy = copies.emplace_back();
        copy.src_subresource = SubresourceLayers{
            .base_level = level,
            .base_layer = 0,
            .num_layers = src.resources.layers,
        };
        copy.dst_subresource = SubresourceLayers{
            .base_level = base.level + level,
            .base_layer = is_dst_3d ? 0 : base.layer,
            .num_layers = is_dst_3d ? 1 : src.resources.layers,
        };
        copy.src_offset = Offset3D{
            .x = 0,
            .y = 0,
            .z = 0,
        };
        copy.dst_offset = Offset3D{
            .x = 0,
            .y = 0,
            .z = is_dst_3d ? base.layer : 0,
        };
        const Extent3D mip_size = AdjustMipSize(dst.size, base.level + level);
        copy.extent = AdjustSamplesSize(mip_size, dst.num_samples);
        if (is_dst_3d) {
            copy.extent.depth = src.size.depth;
        }
        copy.extent.width = std::max<u32>((copy.extent.width * up_scale) >> down_shift, 1);
        if (both_2d) {
            copy.extent.height = std::max<u32>((copy.extent.height * up_scale) >> down_shift, 1);
        }
    }
    return copies;
}

auto MipSize(Extent3D size, u32 level) -> Extent3D { return AdjustMipSize(size, level); }
}  // namespace render::texture::utils