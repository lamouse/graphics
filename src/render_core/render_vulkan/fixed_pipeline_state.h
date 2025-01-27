// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <type_traits>

#include "common/bit_field.hpp"
#include "common/common_types.hpp"
#include "render_core/surface.hpp"

namespace render::vulkan {

enum class PrimitiveTopology : u32 {
    Points = 0x0,
    Lines = 0x1,
    LineLoop = 0x2,
    LineStrip = 0x3,
    Triangles = 0x4,
    TriangleStrip = 0x5,
    TriangleFan = 0x6,
    Quads = 0x7,
    QuadStrip = 0x8,
    Polygon = 0x9,
    LinesAdjacency = 0xA,
    LineStripAdjacency = 0xB,
    TrianglesAdjacency = 0xC,
    TriangleStripAdjacency = 0xD,
    Patches = 0xE,
};

struct VertexArray {
        union {
                BitField<0, 16, u32> start;
                BitField<16, 12, u32> count;
                BitField<28, 3, PrimitiveTopology> topology;
        };
};

// None Engine
enum class EngineHint : u32 {
    None = 0x0,
    OnHLEMacro = 0x1,
};

enum class MsaaMode : u32 {
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

struct DynamicFeatures {
        bool has_extended_dynamic_state;
        bool has_extended_dynamic_state_2;
        bool has_extended_dynamic_state_2_extra;
        bool has_extended_dynamic_state_3_blend;
        bool has_extended_dynamic_state_3_enables;
        bool has_dynamic_vertex_input;
};

struct FixedPipelineState {
        std::array<surface::PixelFormat, 8> color_formats;
        surface::PixelFormat depth_format;
        u32 alpha_test_ref;
        u32 point_size;
        std::array<u16, 16> viewport_swizzles;
        int depth_enabled;
        MsaaMode msaa_mode;
        int xfb_state;
        [[nodiscard]] auto Hash() const noexcept -> size_t;

        auto operator==(const FixedPipelineState& rhs) const noexcept -> bool;

        auto operator!=(const FixedPipelineState& rhs) const noexcept -> bool {
            return !operator==(rhs);
        }

        [[nodiscard]] auto Size() const noexcept -> size_t { return sizeof(*this); }
};
static_assert(std::has_unique_object_representations_v<FixedPipelineState>);
static_assert(std::is_trivially_copyable_v<FixedPipelineState>);
static_assert(std::is_trivially_constructible_v<FixedPipelineState>);

}  // namespace render::vulkan

namespace std {

template <>
struct hash<render::vulkan::FixedPipelineState> {
        auto operator()(const render::vulkan::FixedPipelineState& k) const noexcept -> size_t {
            return k.Hash();
        }
};

}  // namespace std
