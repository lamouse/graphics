// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-FileCopyrightText: Ryujinx Team and Contributors
// SPDX-License-Identifier: GPL-2.0-or-later AND MIT

// This files contains code from Ryujinx
// A copy of the code can be obtained from https://github.com/Ryujinx/Ryujinx
// The sections using code from Ryujinx are marked with a link to the original version

#include <algorithm>
#include <array>
#include <numeric>
#include <optional>
#include <span>
#include <vector>

#include "surface.hpp"
#include "util.hpp"
#include "common/div_ceil.hpp"
namespace render::texture {
using surface::BytesPerBlock;
using surface::DefaultBlockHeight;
using surface::DefaultBlockWidth;
namespace {
struct LevelInfo {
        Extent3D size;
        Extent3D block;
        Extent2D tile_size;
        u32 bpp_log2;
        u32 num_levels;
};
constexpr u32 GOB_SIZE_X = 64;
constexpr u32 GOB_SIZE_Y = 8;
constexpr u32 GOB_SIZE_Z = 1;
constexpr u32 GOB_SIZE = GOB_SIZE_X * GOB_SIZE_Y * GOB_SIZE_Z;
constexpr u32 GOB_SIZE_X_SHIFT = 6;
constexpr u32 GOB_SIZE_Y_SHIFT = 3;
constexpr u32 GOB_SIZE_Z_SHIFT = 0;
constexpr u32 GOB_SIZE_SHIFT = GOB_SIZE_X_SHIFT + GOB_SIZE_Y_SHIFT + GOB_SIZE_Z_SHIFT;
template <typename T>
    requires std::is_unsigned_v<T>
[[nodiscard]] constexpr T AlignUpLog2(T value, size_t align_log2) {
    return static_cast<T>((value + ((1ULL << align_log2) - 1)) >> align_log2 << align_log2);
}
[[nodiscard]] constexpr Extent2D DefaultBlockSize(surface::PixelFormat format) {
    return {DefaultBlockWidth(format), DefaultBlockHeight(format)};
}
[[nodiscard]] constexpr u32 BytesPerBlockLog2(u32 bytes_per_block) {
    return std::countl_zero(bytes_per_block) ^ 0x1F;
}
[[nodiscard]] constexpr u32 AdjustTileSize(u32 shift, u32 unit_factor, u32 dimension) {
    if (shift == 0) {
        return 0;
    }
    u32 x = unit_factor << (shift - 1);
    if (x >= dimension) {
        while (--shift) {
            x >>= 1;
            if (x < dimension) {
                break;
            }
        }
    }
    return shift;
}
[[nodiscard]] constexpr u32 AdjustMipSize(u32 size, u32 level) {
    return std::max<u32>(size >> level, 1);
}
[[nodiscard]] constexpr u32 AdjustSize(u32 size, u32 level, u32 block_size) {
    return common::DivCeil(AdjustMipSize(size, level), block_size);
}

[[nodiscard]] constexpr Extent3D NumLevelBlocks(const LevelInfo& info, u32 level) {
    return Extent3D{
        .width = AdjustSize(info.size.width, level, info.tile_size.width) << info.bpp_log2,
        .height = AdjustSize(info.size.height, level, info.tile_size.height),
        .depth = AdjustMipSize(info.size.depth, level),
    };
}

[[nodiscard]] constexpr Extent3D TileShift(const LevelInfo& info, u32 level) {
    if (level == 0 && info.num_levels == 1) {
        return Extent3D{
            .width = info.block.width,
            .height = info.block.height,
            .depth = info.block.depth,
        };
    }
    const Extent3D blocks = NumLevelBlocks(info, level);
    return Extent3D{
        .width = AdjustTileSize(info.block.width, GOB_SIZE_X, blocks.width),
        .height = AdjustTileSize(info.block.height, GOB_SIZE_Y, blocks.height),
        .depth = AdjustTileSize(info.block.depth, GOB_SIZE_Z, blocks.depth),
    };
}
[[nodiscard]] constexpr bool IsSmallerThanGobSize(Extent3D num_tiles, Extent2D gob,
                                                  u32 block_depth) {
    return num_tiles.width <= (1U << gob.width) || num_tiles.height <= (1U << gob.height) ||
           num_tiles.depth < (1U << block_depth);
}

[[nodiscard]] constexpr Extent2D GobSize(u32 bpp_log2, u32 block_height) {
    return Extent2D{
        .width = GOB_SIZE_X_SHIFT - bpp_log2,
        .height = GOB_SIZE_Y_SHIFT + block_height,
    };
}
[[nodiscard]] constexpr Extent2D NumGobs(const LevelInfo& info, u32 level) {
    const Extent3D blocks = NumLevelBlocks(info, level);
    const Extent2D gobs{
        .width = common::DivCeilLog2(blocks.width, GOB_SIZE_X_SHIFT),
        .height = common::DivCeilLog2(blocks.height, GOB_SIZE_Y_SHIFT),
    };
    const Extent2D gob = GobSize(info.bpp_log2, info.block.height);
    const u32 alignment = 0;
    return Extent2D{
        .width = AlignUpLog2(gobs.width, alignment),
        .height = gobs.height,
    };
}

[[nodiscard]] constexpr Extent3D LevelTiles(const LevelInfo& info, u32 level) {
    const Extent3D blocks = NumLevelBlocks(info, level);
    const Extent3D tile_shift = TileShift(info, level);
    const Extent2D gobs = NumGobs(info, level);
    return Extent3D{
        .width = common::DivCeilLog2(gobs.width, tile_shift.width),
        .height = common::DivCeilLog2(gobs.height, tile_shift.height),
        .depth = common::DivCeilLog2(blocks.depth, tile_shift.depth),
    };
}
[[nodiscard]] constexpr u32 CalculateLevelSize(const LevelInfo& info, u32 level) {
    const Extent3D tile_shift = TileShift(info, level);
    const Extent3D tiles = LevelTiles(info, level);
    const u32 num_tiles = tiles.width * tiles.height * tiles.depth;
    const u32 shift = GOB_SIZE_SHIFT + tile_shift.width + tile_shift.height + tile_shift.depth;
    return num_tiles << shift;
}

[[nodiscard]] constexpr LevelInfo MakeLevelInfo(surface::PixelFormat format, Extent3D size,
                                                Extent3D block,
                                                u32 num_levels) {
    const u32 bytes_per_block = BytesPerBlock(format);
    return {
        .size =
            {
                .width = size.width,
                .height = size.height,
                .depth = size.depth,
            },
        .block = block,
        .tile_size = DefaultBlockSize(format),
        .bpp_log2 = BytesPerBlockLog2(bytes_per_block),
        .num_levels = num_levels,
    };
}

[[nodiscard]] constexpr LevelInfo MakeLevelInfo(const ImageInfo& info) {
    return MakeLevelInfo(info.format, info.size, info.block_or_pitch.block,
                         info.resources.levels);
}
[[nodiscard]] constexpr u32 CalculateLevelOffset(surface::PixelFormat format, Extent3D size,
                                                 Extent3D block,
                                                 u32 level) {
    const LevelInfo info = MakeLevelInfo(format, size, block, level);
    u32 offset = 0;
    for (u32 current_level = 0; current_level < level; ++current_level) {
        offset += CalculateLevelSize(info, current_level);
    }
    return offset;
}
}  // namespace

auto CalculateGuestSizeInBytes(const ImageInfo& info) noexcept -> u32 {
    if (info.type == ImageType::Buffer) {
        return info.size.width * BytesPerBlock(info.format);
    }
    if (info.type == ImageType::Linear) {
        return info.block_or_pitch.pitch * common::DivCeil(info.size.height, DefaultBlockHeight(info.format));
    }
    return CalculateLayerSize(info);
}

auto CalculateLayerSize(const ImageInfo& info) noexcept -> u32 {
    assert(info.type != ImageType::Linear);
    return CalculateLevelOffset(info.format, info.size, info.block_or_pitch.block,
                                info.resources.levels);
}

}  // namespace render::texture
