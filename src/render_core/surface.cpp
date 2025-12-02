#include "common/common_types.hpp"
#include "surface.hpp"
#include <spdlog/spdlog.h>
#include "common/settings.hpp"
namespace render::surface {

auto SurfaceTargetIsLayered(SurfaceTarget target) -> bool {
    switch (target) {
        case SurfaceTarget::Texture1D:
        case SurfaceTarget::TextureBuffer:
        case SurfaceTarget::Texture2D:
        case SurfaceTarget::Texture3D:
            return false;
        case SurfaceTarget::Texture1DArray:
        case SurfaceTarget::Texture2DArray:
        case SurfaceTarget::TextureCubemap:
        case SurfaceTarget::TextureCubeArray:
            return true;
        default:
            SPDLOG_ERROR("Unimplemented layered surface_target={}", static_cast<int>(target));
            assert(false);
            return false;
    }
}

auto SurfaceTargetIsArray(SurfaceTarget target) -> bool {
    switch (target) {
        case SurfaceTarget::Texture1D:
        case SurfaceTarget::TextureBuffer:
        case SurfaceTarget::Texture2D:
        case SurfaceTarget::Texture3D:
        case SurfaceTarget::TextureCubemap:
            return false;
        case SurfaceTarget::Texture1DArray:
        case SurfaceTarget::Texture2DArray:
        case SurfaceTarget::TextureCubeArray:
            return true;
        default:
            SPDLOG_ERROR("Unimplemented array surface_target={}", static_cast<int>(target));
            assert(false);
            return false;
    }
}

auto GetFormatType(PixelFormat pixel_format) -> SurfaceType {
    if (pixel_format < PixelFormat::MaxColorFormat) {
        return SurfaceType::ColorTexture;
    }
    if (pixel_format < PixelFormat::MaxDepthFormat) {
        return SurfaceType::Depth;
    }
    if (pixel_format < PixelFormat::MaxStencilFormat) {
        return SurfaceType::Stencil;
    }
    if (pixel_format < PixelFormat::MaxDepthStencilFormat) {
        return SurfaceType::DepthStencil;
    }

    // TODO(Subv): Implement the other formats
    assert(false);

    return SurfaceType::Invalid;
}

bool IsPixelFormatASTC(PixelFormat format) {
    switch (format) {
        case PixelFormat::ASTC_2D_4X4_UNORM:
        case PixelFormat::ASTC_2D_5X4_UNORM:
        case PixelFormat::ASTC_2D_5X5_UNORM:
        case PixelFormat::ASTC_2D_8X8_UNORM:
        case PixelFormat::ASTC_2D_8X5_UNORM:
        case PixelFormat::ASTC_2D_4X4_SRGB:
        case PixelFormat::ASTC_2D_5X4_SRGB:
        case PixelFormat::ASTC_2D_5X5_SRGB:
        case PixelFormat::ASTC_2D_8X8_SRGB:
        case PixelFormat::ASTC_2D_8X5_SRGB:
        case PixelFormat::ASTC_2D_10X8_UNORM:
        case PixelFormat::ASTC_2D_10X8_SRGB:
        case PixelFormat::ASTC_2D_6X6_UNORM:
        case PixelFormat::ASTC_2D_6X6_SRGB:
        case PixelFormat::ASTC_2D_10X6_UNORM:
        case PixelFormat::ASTC_2D_10X6_SRGB:
        case PixelFormat::ASTC_2D_10X5_UNORM:
        case PixelFormat::ASTC_2D_10X5_SRGB:
        case PixelFormat::ASTC_2D_10X10_UNORM:
        case PixelFormat::ASTC_2D_10X10_SRGB:
        case PixelFormat::ASTC_2D_12X10_UNORM:
        case PixelFormat::ASTC_2D_12X10_SRGB:
        case PixelFormat::ASTC_2D_12X12_UNORM:
        case PixelFormat::ASTC_2D_12X12_SRGB:
        case PixelFormat::ASTC_2D_8X6_UNORM:
        case PixelFormat::ASTC_2D_8X6_SRGB:
        case PixelFormat::ASTC_2D_6X5_UNORM:
        case PixelFormat::ASTC_2D_6X5_SRGB:
            return true;
        default:
            return false;
    }
}

auto IsPixelFormatBCn(PixelFormat format) -> bool {
    switch (format) {
        case PixelFormat::BC1_RGBA_UNORM:
        case PixelFormat::BC2_UNORM:
        case PixelFormat::BC3_UNORM:
        case PixelFormat::BC4_UNORM:
        case PixelFormat::BC4_SNORM:
        case PixelFormat::BC5_UNORM:
        case PixelFormat::BC5_SNORM:
        case PixelFormat::BC1_RGBA_SRGB:
        case PixelFormat::BC2_SRGB:
        case PixelFormat::BC3_SRGB:
        case PixelFormat::BC7_UNORM:
        case PixelFormat::BC6H_UFLOAT:
        case PixelFormat::BC6H_SFLOAT:
        case PixelFormat::BC7_SRGB:
            return true;
        default:
            return false;
    }
}

bool IsPixelFormatSRGB(PixelFormat format) {
    switch (format) {
        case PixelFormat::A8B8G8R8_SRGB:
        case PixelFormat::B8G8R8A8_SRGB:
        case PixelFormat::BC1_RGBA_SRGB:
        case PixelFormat::BC2_SRGB:
        case PixelFormat::BC3_SRGB:
        case PixelFormat::BC7_SRGB:
        case PixelFormat::ASTC_2D_4X4_SRGB:
        case PixelFormat::ASTC_2D_8X8_SRGB:
        case PixelFormat::ASTC_2D_8X5_SRGB:
        case PixelFormat::ASTC_2D_5X4_SRGB:
        case PixelFormat::ASTC_2D_5X5_SRGB:
        case PixelFormat::ASTC_2D_10X6_SRGB:
        case PixelFormat::ASTC_2D_10X8_SRGB:
        case PixelFormat::ASTC_2D_6X6_SRGB:
        case PixelFormat::ASTC_2D_10X5_SRGB:
        case PixelFormat::ASTC_2D_10X10_SRGB:
        case PixelFormat::ASTC_2D_12X12_SRGB:
        case PixelFormat::ASTC_2D_12X10_SRGB:
        case PixelFormat::ASTC_2D_8X6_SRGB:
        case PixelFormat::ASTC_2D_6X5_SRGB:
            return true;
        default:
            return false;
    }
}

bool IsPixelFormatInteger(PixelFormat format) {
    switch (format) {
        case PixelFormat::A8B8G8R8_SINT:
        case PixelFormat::A8B8G8R8_UINT:
        case PixelFormat::A2B10G10R10_UINT:
        case PixelFormat::R8_SINT:
        case PixelFormat::R8_UINT:
        case PixelFormat::R16G16B16A16_SINT:
        case PixelFormat::R16G16B16A16_UINT:
        case PixelFormat::R32G32B32A32_UINT:
        case PixelFormat::R32G32B32A32_SINT:
        case PixelFormat::R32G32_SINT:
        case PixelFormat::R16_UINT:
        case PixelFormat::R16_SINT:
        case PixelFormat::R16G16_UINT:
        case PixelFormat::R16G16_SINT:
        case PixelFormat::R8G8_SINT:
        case PixelFormat::R8G8_UINT:
        case PixelFormat::R32G32_UINT:
        case PixelFormat::R32_UINT:
        case PixelFormat::R32_SINT:
            return true;
        default:
            return false;
    }
}

bool IsPixelFormatSignedInteger(PixelFormat format) {
    switch (format) {
        case PixelFormat::A8B8G8R8_SINT:
        case PixelFormat::R8_SINT:
        case PixelFormat::R16G16B16A16_SINT:
        case PixelFormat::R32G32B32A32_SINT:
        case PixelFormat::R32G32_SINT:
        case PixelFormat::R16_SINT:
        case PixelFormat::R16G16_SINT:
        case PixelFormat::R8G8_SINT:
        case PixelFormat::R32_SINT:
            return true;
        default:
            return false;
    }
}

auto PixelComponentSizeBitsInteger(PixelFormat format) -> std::size_t {
    switch (format) {
        case PixelFormat::A8B8G8R8_SINT:
        case PixelFormat::A8B8G8R8_UINT:
        case PixelFormat::R8_SINT:
        case PixelFormat::R8_UINT:
        case PixelFormat::R8G8_SINT:
        case PixelFormat::R8G8_UINT:
            return 8;
        case PixelFormat::A2B10G10R10_UINT:
            return 10;
        case PixelFormat::R16G16B16A16_SINT:
        case PixelFormat::R16G16B16A16_UINT:
        case PixelFormat::R16_UINT:
        case PixelFormat::R16_SINT:
        case PixelFormat::R16G16_UINT:
        case PixelFormat::R16G16_SINT:
            return 16;
        case PixelFormat::R32G32B32A32_UINT:
        case PixelFormat::R32G32B32A32_SINT:
        case PixelFormat::R32G32_SINT:
        case PixelFormat::R32G32_UINT:
        case PixelFormat::R32_UINT:
        case PixelFormat::R32_SINT:
            return 32;
        default:
            return 0;
    }
}

auto TranscodedAstcSize(u64 base_size, PixelFormat format) -> u64 {
    constexpr u64 RGBA8_PIXEL_SIZE = 4;
    const u64 base_block_size = static_cast<u64>(DefaultBlockWidth(format)) *
                                static_cast<u64>(DefaultBlockHeight(format)) * RGBA8_PIXEL_SIZE;
    const u64 uncompressed_size = (base_size * base_block_size) / BytesPerBlock(format);
    auto astc_recompression = settings::values.astc_recompression.GetValue();
    switch (astc_recompression) {
        case settings::enums::AstcRecompression::Bc1:
            return uncompressed_size / 8;
        case settings::enums::AstcRecompression::Bc3:
            return uncompressed_size / 4;
        default:
            return uncompressed_size;
    }
}

}  // namespace render::surface
