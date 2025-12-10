#pragma once
#include "common/common_types.hpp"
#include "common/common_funcs.hpp"
#include "common/slot_vector.hpp"
namespace render::texture {

constexpr size_t NUM_RT = 8;
constexpr size_t MAX_MIP_LEVELS = 14;
constexpr common::SlotId CORRUPT_ID{0xfffffffe};
using ImageId = common::SlotId;
using ImageMapId = common::SlotId;
using ImageViewId = common::SlotId;
using SamplerId = common::SlotId;
using FramebufferId = common::SlotId;

/// Fake image ID for null image views
constexpr ImageId NULL_IMAGE_ID{0};
/// Image view ID for null descriptors
constexpr ImageViewId NULL_IMAGE_VIEW_ID{0};
/// Sampler ID for bugged sampler ids
constexpr SamplerId NULL_SAMPLER_ID{0};

enum class ImageType : std::uint8_t { e2D, e3D };

enum class ImageViewType : std::uint8_t { e2D, Cube, e3D, e2DArray, CubeArray };
constexpr size_t NUM_IMAGE_VIEW_TYPES = 5;

struct Offset2D {
        constexpr auto operator<=>(const Offset2D&) const noexcept = default;

        s32 x;
        s32 y;
};

struct Offset3D {
        constexpr auto operator<=>(const Offset3D&) const noexcept = default;

        s32 x;
        s32 y;
        s32 z;
};

struct Region2D {
        constexpr auto operator<=>(const Region2D&) const noexcept = default;

        Offset2D start;
        Offset2D end;
};

struct Extent2D {
        constexpr auto operator<=>(const Extent2D&) const noexcept = default;

        u32 width;
        u32 height;
};

struct Extent3D {
        constexpr auto operator<=>(const Extent3D&) const noexcept = default;

        u32 width;
        u32 height;
        u32 depth;
};

struct SubresourceLayers {
        s32 base_level = 0;
        s32 base_layer = 0;
        s32 num_layers = 1;
};

struct SubresourceBase {
        constexpr auto operator<=>(const SubresourceBase&) const noexcept = default;

        s32 level = 0;
        s32 layer = 0;
};

struct SubresourceExtent {
        constexpr auto operator<=>(const SubresourceExtent&) const noexcept = default;

        s32 levels = 1;
        s32 layers = 1;
};

struct SubresourceRange {
        constexpr auto operator<=>(const SubresourceRange&) const noexcept = default;

        SubresourceBase base;
        SubresourceExtent extent;
};

struct ImageCopy {
        SubresourceLayers src_subresource;
        SubresourceLayers dst_subresource;
        Offset3D src_offset;
        Offset3D dst_offset;
        Extent3D extent;
};

struct BufferImageCopy {
        size_t buffer_offset{};
        size_t buffer_size{};
        u32 buffer_row_length{};
        u32 buffer_image_height{};
        SubresourceLayers image_subresource;
        Offset3D image_offset{};
        Extent3D image_extent{};
};

struct BufferCopy {
        u64 src_offset;
        u64 dst_offset;
        size_t size;
};

struct SwizzleParameters {
        Extent3D num_tiles;
        Extent3D block;
        size_t buffer_offset;
        s32 level;
};

enum class MsaaMode : std::uint8_t {
    Msaa1x1 = 0,
    Msaa2x1 = 1,
    Msaa2x2 = 2,
    Msaa4x2 = 3,
    Msaa4x2_D3D = 4,
    Msaa2x1_D3D = 5,
    Msaa4x4 = 6,
    Msaa2x2_VC4 = 8,
    Msaa2x2_VC12 = 9,
    Msaa4x2_VC8 = 10,
    Msaa4x2_VC24 = 11,
};

}  // namespace render::texture

// 在全局命名空间或 std 内（推荐在 std 中特化）
namespace std {
template <>
struct hash<render::texture::Extent3D> {
        auto operator()(const render::texture::Extent3D& ext) const noexcept -> size_t {
            // 使用哈希组合算法（推荐 FNV-1a 风格）
            size_t h1 = std::hash<u32>{}(ext.width);
            size_t h2 = std::hash<u32>{}(ext.height);
            size_t h3 = std::hash<u32>{}(ext.depth);
            return h1 ^ (h2 << 1U) ^ (h3 << 2U);
        }
};
}  // namespace std