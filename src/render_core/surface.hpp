#pragma once

namespace render::surface {

enum class PixelFormat {
    A8B8G8R8_UNORM,
    A8B8G8R8_SNORM,
    A8B8G8R8_SINT,
    A8B8G8R8_UINT,
    R5G6B5_UNORM,
    B5G6R5_UNORM,
    A1R5G5B5_UNORM,
    A2B10G10R10_UNORM,
    A2B10G10R10_UINT,
    A2R10G10B10_UNORM,
    A1B5G5R5_UNORM,
    A5B5G5R1_UNORM,
    R8_UNORM,
    R8_SNORM,
    R8_SINT,
    R8_UINT,
    R16G16B16A16_FLOAT,
    R16G16B16A16_UNORM,
    R16G16B16A16_SNORM,
    R16G16B16A16_SINT,
    R16G16B16A16_UINT,
    B10G11R11_FLOAT,
    R32G32B32A32_UINT,
    BC1_RGBA_UNORM,
    BC2_UNORM,
    BC3_UNORM,
    BC4_UNORM,
    BC4_SNORM,
    BC5_UNORM,
    BC5_SNORM,
    BC7_UNORM,
    BC6H_UFLOAT,
    BC6H_SFLOAT,
    ASTC_2D_4X4_UNORM,
    B8G8R8A8_UNORM,
    R32G32B32A32_FLOAT,
    R32G32B32A32_SINT,
    R32G32_FLOAT,
    R32G32_SINT,
    R32_FLOAT,
    R16_FLOAT,
    R16_UNORM,
    R16_SNORM,
    R16_UINT,
    R16_SINT,
    R16G16_UNORM,
    R16G16_FLOAT,
    R16G16_UINT,
    R16G16_SINT,
    R16G16_SNORM,
    R32G32B32_FLOAT,
    A8B8G8R8_SRGB,
    R8G8_UNORM,
    R8G8_SNORM,
    R8G8_SINT,
    R8G8_UINT,
    R32G32_UINT,
    R16G16B16X16_FLOAT,
    R32_UINT,
    R32_SINT,
    ASTC_2D_8X8_UNORM,
    ASTC_2D_8X5_UNORM,
    ASTC_2D_5X4_UNORM,
    B8G8R8A8_SRGB,
    BC1_RGBA_SRGB,
    BC2_SRGB,
    BC3_SRGB,
    BC7_SRGB,
    A4B4G4R4_UNORM,
    G4R4_UNORM,
    ASTC_2D_4X4_SRGB,
    ASTC_2D_8X8_SRGB,
    ASTC_2D_8X5_SRGB,
    ASTC_2D_5X4_SRGB,
    ASTC_2D_5X5_UNORM,
    ASTC_2D_5X5_SRGB,
    ASTC_2D_10X8_UNORM,
    ASTC_2D_10X8_SRGB,
    ASTC_2D_6X6_UNORM,
    ASTC_2D_6X6_SRGB,
    ASTC_2D_10X6_UNORM,
    ASTC_2D_10X6_SRGB,
    ASTC_2D_10X5_UNORM,
    ASTC_2D_10X5_SRGB,
    ASTC_2D_10X10_UNORM,
    ASTC_2D_10X10_SRGB,
    ASTC_2D_12X10_UNORM,
    ASTC_2D_12X10_SRGB,
    ASTC_2D_12X12_UNORM,
    ASTC_2D_12X12_SRGB,
    ASTC_2D_8X6_UNORM,
    ASTC_2D_8X6_SRGB,
    ASTC_2D_6X5_UNORM,
    ASTC_2D_6X5_SRGB,
    E5B9G9R9_FLOAT,

    MaxColorFormat,

    // Depth formats
    D32_FLOAT = MaxColorFormat,
    D16_UNORM,
    X8_D24_UNORM,

    MaxDepthFormat,

    // Stencil formats
    S8_UINT = MaxDepthFormat,
    MaxStencilFormat,

    // DepthStencil formats
    D24_UNORM_S8_UINT = MaxStencilFormat,
    S8_UINT_D24_UNORM,
    D32_FLOAT_S8_UINT,

    MaxDepthStencilFormat,

    Max = MaxDepthStencilFormat,
    Invalid = 255,
};
}