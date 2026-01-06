module;
#include <algorithm>
#include <string>

#include <fmt/format.h>
module render.texture.formatter;
import render.texture.types;
import render.texture.render_targets;
import render.texture.sample.helper;
import render.texture.image;
import render.texture.image_view_base;
import render.surface.format;
import common;

template <>
struct fmt::formatter<render::surface::PixelFormat> : fmt::formatter<fmt::string_view> {
        template <typename FormatContext>
        auto format(render::surface::PixelFormat format, FormatContext& ctx) const {
            using render::surface::PixelFormat;
            const string_view name = [format] {
                switch (format) {
                    case PixelFormat::A8B8G8R8_UNORM:
                        return "A8B8G8R8_UNORM";
                    case PixelFormat::A8B8G8R8_SNORM:
                        return "A8B8G8R8_SNORM";
                    case PixelFormat::A8B8G8R8_SINT:
                        return "A8B8G8R8_SINT";
                    case PixelFormat::A8B8G8R8_UINT:
                        return "A8B8G8R8_UINT";
                    case PixelFormat::A2B10G10R10_UNORM:
                        return "A2B10G10R10_UNORM";
                    case PixelFormat::A2B10G10R10_UINT:
                        return "A2B10G10R10_UINT";
                    case PixelFormat::A2R10G10B10_UNORM:
                        return "A2R10G10B10_UNORM";
                    case PixelFormat::R8_UNORM:
                        return "R8_UNORM";
                    case PixelFormat::R8_SNORM:
                        return "R8_SNORM";
                    case PixelFormat::R8_SINT:
                        return "R8_SINT";
                    case PixelFormat::R8_UINT:
                        return "R8_UINT";
                    case PixelFormat::R16G16B16A16_FLOAT:
                        return "R16G16B16A16_FLOAT";
                    case PixelFormat::R16G16B16A16_UNORM:
                        return "R16G16B16A16_UNORM";
                    case PixelFormat::R16G16B16A16_SNORM:
                        return "R16G16B16A16_SNORM";
                    case PixelFormat::R16G16B16A16_SINT:
                        return "R16G16B16A16_SINT";
                    case PixelFormat::R16G16B16A16_UINT:
                        return "R16G16B16A16_UINT";
                    case PixelFormat::B10G11R11_FLOAT:
                        return "B10G11R11_FLOAT";
                    case PixelFormat::R32G32B32A32_UINT:
                        return "R32G32B32A32_UINT";
                    case PixelFormat::BC1_RGBA_UNORM:
                        return "BC1_RGBA_UNORM";
                    case PixelFormat::BC2_UNORM:
                        return "BC2_UNORM";
                    case PixelFormat::BC3_UNORM:
                        return "BC3_UNORM";
                    case PixelFormat::BC4_UNORM:
                        return "BC4_UNORM";
                    case PixelFormat::BC4_SNORM:
                        return "BC4_SNORM";
                    case PixelFormat::BC5_UNORM:
                        return "BC5_UNORM";
                    case PixelFormat::BC5_SNORM:
                        return "BC5_SNORM";
                    case PixelFormat::BC7_UNORM:
                        return "BC7_UNORM";
                    case PixelFormat::BC6H_UFLOAT:
                        return "BC6H_UFLOAT";
                    case PixelFormat::BC6H_SFLOAT:
                        return "BC6H_SFLOAT";
                    case PixelFormat::ASTC_2D_4X4_UNORM:
                        return "ASTC_2D_4X4_UNORM";
                    case PixelFormat::B8G8R8A8_UNORM:
                        return "B8G8R8A8_UNORM";
                    case PixelFormat::R32G32B32A32_FLOAT:
                        return "R32G32B32A32_FLOAT";
                    case PixelFormat::R32G32B32A32_SINT:
                        return "R32G32B32A32_SINT";
                    case PixelFormat::R32G32_FLOAT:
                        return "R32G32_FLOAT";
                    case PixelFormat::R32G32_SINT:
                        return "R32G32_SINT";
                    case PixelFormat::R32_FLOAT:
                        return "R32_FLOAT";
                    case PixelFormat::R16_FLOAT:
                        return "R16_FLOAT";
                    case PixelFormat::R16_UNORM:
                        return "R16_UNORM";
                    case PixelFormat::R16_SNORM:
                        return "R16_SNORM";
                    case PixelFormat::R16_UINT:
                        return "R16_UINT";
                    case PixelFormat::R16_SINT:
                        return "R16_SINT";
                    case PixelFormat::R16G16_UNORM:
                        return "R16G16_UNORM";
                    case PixelFormat::R16G16_FLOAT:
                        return "R16G16_FLOAT";
                    case PixelFormat::R16G16_UINT:
                        return "R16G16_UINT";
                    case PixelFormat::R16G16_SINT:
                        return "R16G16_SINT";
                    case PixelFormat::R16G16_SNORM:
                        return "R16G16_SNORM";
                    case PixelFormat::R32G32B32_FLOAT:
                        return "R32G32B32_FLOAT";
                    case PixelFormat::A8B8G8R8_SRGB:
                        return "A8B8G8R8_SRGB";
                    case PixelFormat::R8G8_UNORM:
                        return "R8G8_UNORM";
                    case PixelFormat::R8G8_SNORM:
                        return "R8G8_SNORM";
                    case PixelFormat::R8G8_SINT:
                        return "R8G8_SINT";
                    case PixelFormat::R8G8_UINT:
                        return "R8G8_UINT";
                    case PixelFormat::R32G32_UINT:
                        return "R32G32_UINT";
                    case PixelFormat::R16G16B16X16_FLOAT:
                        return "R16G16B16X16_FLOAT";
                    case PixelFormat::R32_UINT:
                        return "R32_UINT";
                    case PixelFormat::R32_SINT:
                        return "R32_SINT";
                    case PixelFormat::ASTC_2D_8X8_UNORM:
                        return "ASTC_2D_8X8_UNORM";
                    case PixelFormat::ASTC_2D_8X5_UNORM:
                        return "ASTC_2D_8X5_UNORM";
                    case PixelFormat::ASTC_2D_5X4_UNORM:
                        return "ASTC_2D_5X4_UNORM";
                    case PixelFormat::B8G8R8A8_SRGB:
                        return "B8G8R8A8_SRGB";
                    case PixelFormat::BC1_RGBA_SRGB:
                        return "BC1_RGBA_SRGB";
                    case PixelFormat::BC2_SRGB:
                        return "BC2_SRGB";
                    case PixelFormat::BC3_SRGB:
                        return "BC3_SRGB";
                    case PixelFormat::BC7_SRGB:
                        return "BC7_SRGB";
                    case PixelFormat::G4R4_UNORM:
                        return "G4R4_UNORM";
                    case PixelFormat::ASTC_2D_4X4_SRGB:
                        return "ASTC_2D_4X4_SRGB";
                    case PixelFormat::ASTC_2D_8X8_SRGB:
                        return "ASTC_2D_8X8_SRGB";
                    case PixelFormat::ASTC_2D_8X5_SRGB:
                        return "ASTC_2D_8X5_SRGB";
                    case PixelFormat::ASTC_2D_5X4_SRGB:
                        return "ASTC_2D_5X4_SRGB";
                    case PixelFormat::ASTC_2D_5X5_UNORM:
                        return "ASTC_2D_5X5_UNORM";
                    case PixelFormat::ASTC_2D_5X5_SRGB:
                        return "ASTC_2D_5X5_SRGB";
                    case PixelFormat::ASTC_2D_10X8_UNORM:
                        return "ASTC_2D_10X8_UNORM";
                    case PixelFormat::ASTC_2D_10X8_SRGB:
                        return "ASTC_2D_10X8_SRGB";
                    case PixelFormat::ASTC_2D_6X6_UNORM:
                        return "ASTC_2D_6X6_UNORM";
                    case PixelFormat::ASTC_2D_6X6_SRGB:
                        return "ASTC_2D_6X6_SRGB";
                    case PixelFormat::ASTC_2D_10X6_UNORM:
                        return "ASTC_2D_10X6_UNORM";
                    case PixelFormat::ASTC_2D_10X6_SRGB:
                        return "ASTC_2D_10X6_SRGB";
                    case PixelFormat::ASTC_2D_10X5_UNORM:
                        return "ASTC_2D_10X5_UNORM";
                    case PixelFormat::ASTC_2D_10X5_SRGB:
                        return "ASTC_2D_10X5_SRGB";
                    case PixelFormat::ASTC_2D_10X10_UNORM:
                        return "ASTC_2D_10X10_UNORM";
                    case PixelFormat::ASTC_2D_10X10_SRGB:
                        return "ASTC_2D_10X10_SRGB";
                    case PixelFormat::ASTC_2D_12X10_UNORM:
                        return "ASTC_2D_12X10_UNORM";
                    case PixelFormat::ASTC_2D_12X10_SRGB:
                        return "ASTC_2D_12X10_SRGB";
                    case PixelFormat::ASTC_2D_12X12_UNORM:
                        return "ASTC_2D_12X12_UNORM";
                    case PixelFormat::ASTC_2D_12X12_SRGB:
                        return "ASTC_2D_12X12_SRGB";
                    case PixelFormat::ASTC_2D_8X6_UNORM:
                        return "ASTC_2D_8X6_UNORM";
                    case PixelFormat::ASTC_2D_8X6_SRGB:
                        return "ASTC_2D_8X6_SRGB";
                    case PixelFormat::ASTC_2D_6X5_UNORM:
                        return "ASTC_2D_6X5_UNORM";
                    case PixelFormat::ASTC_2D_6X5_SRGB:
                        return "ASTC_2D_6X5_SRGB";
                    case PixelFormat::E5B9G9R9_FLOAT:
                        return "E5B9G9R9_FLOAT";
                    case PixelFormat::D32_FLOAT:
                        return "D32_FLOAT";
                    case PixelFormat::D16_UNORM:
                        return "D16_UNORM";
                    case PixelFormat::X8_D24_UNORM:
                        return "X8_D24_UNORM";
                    case PixelFormat::S8_UINT:
                        return "S8_UINT";
                    case PixelFormat::D24_UNORM_S8_UINT:
                        return "D24_UNORM_S8_UINT";
                    case PixelFormat::S8_UINT_D24_UNORM:
                        return "S8_UINT_D24_UNORM";
                    case PixelFormat::D32_FLOAT_S8_UINT:
                        return "D32_FLOAT_S8_UINT";
                    case PixelFormat::MaxDepthStencilFormat:
                    case PixelFormat::Invalid:
                        return "Invalid";
                }
                return "Invalid";
            }();
            return formatter<string_view>::format(name, ctx);
        }
};

template <>
struct fmt::formatter<render::texture::ImageType> : fmt::formatter<fmt::string_view> {
        template <typename FormatContext>
        auto format(render::texture::ImageType type, FormatContext& ctx) const {
            const string_view name = [type] {
                using render::texture::ImageType;
                switch (type) {
                    case ImageType::e2D:
                        return "2D";
                    case ImageType::e3D:
                        return "3D";
                }
                return "Invalid";
            }();
            return formatter<string_view>::format(name, ctx);
        }
};

template <>
struct fmt::formatter<render::texture::Extent3D> {
        constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

        template <typename FormatContext>
        auto format(const render::texture::Extent3D& extent, FormatContext& ctx) const {
            return fmt::format_to(ctx.out(), "{{{}, {}, {}}}", extent.width, extent.height,
                                  extent.depth);
        }
};


namespace render::texture {

auto Name(const ImageInfo& image) -> std::string {
    u32 width = image.size.width;
    u32 height = image.size.height;
    const u32 depth = image.size.depth;

    std::string resource;
    if (image.num_samples > 1) {
        const auto [samples_x, samples_y] = render::texture::SamplesLog2(image.num_samples);
        width >>= samples_x;
        height >>= samples_y;
        resource += fmt::format(":{}xMSAA", image.num_samples);
    }
    switch (image.type) {
        case ImageType::e2D:
            return fmt::format("Image 2D  {}x{}{}", width, height, resource);
        case ImageType::e3D:
            return fmt::format("Image 2D  {}x{}x{}{}", width, height, depth, resource);
    }
    return "Invalid";
}

auto Name(const ImageViewBase& image_view) -> std::string {
    const u32 width = image_view.size.width;
    const u32 height = image_view.size.height;
    const u32 depth = image_view.size.depth;
    const u32 num_levels = image_view.range.extent.levels;
    const u32 num_layers = image_view.range.extent.layers;

    const std::string level = num_levels > 1 ? fmt::format(":{}", num_levels) : "";
    switch (image_view.type) {
        case ImageViewType::e2D:
            return fmt::format("ImageView 2D  {}x{}{}", width, height, level);
        case ImageViewType::Cube:
            return fmt::format("ImageView Cube  {}x{}{}", width, height, level);
        case ImageViewType::e3D:
            return fmt::format("ImageView 3D  {}x{}x{}{}", width, height, depth, level);
        case ImageViewType::e2DArray:
            return fmt::format("ImageView 2DArray {}x{}{}|{}", width, height, level, num_layers);
        case ImageViewType::CubeArray:
            return fmt::format("ImageView CubeArray  {}x{}{}|{}", width, height, level, num_layers);
    }
    return "Invalid";
}

auto Name(const RenderTargets& render_targets) -> std::string {
    std::string_view debug_prefix;
    const auto num_color = std::ranges::count_if(
        render_targets.color_buffer_ids, [](ImageViewId id) { return static_cast<bool>(id); });
    if (render_targets.depth_buffer_id) {
        debug_prefix = num_color > 0 ? "R" : "Z";
    } else {
        debug_prefix = num_color > 0 ? "C" : "X";
    }
    const Extent2D size = render_targets.size;
    if (num_color > 0) {
        return fmt::format("Framebuffer {}{} {}x{}", debug_prefix, num_color, size.width,
                           size.height);
    } else {
        return fmt::format("Framebuffer {} {}x{}", debug_prefix, size.width, size.height);
    }
}

}  // namespace render::texture
