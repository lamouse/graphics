// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <type_traits>
#include <functional>
#include "common/bit_field.hpp"
#include "common/common_types.hpp"
#include "render_core/surface.hpp"
namespace render {

enum class PrimitiveTopology : u32 {
    Points = 0x0,
    Lines = 0x1,
    LineStrip = 0x3,
    Triangles = 0x4,
    TriangleStrip = 0x5,
    TriangleFan = 0x6,
    LinesAdjacency = 0xA,
    LineStripAdjacency = 0xB,
    TrianglesAdjacency = 0xC,
    TriangleStripAdjacency = 0xD,
    Patches = 0xE,
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
        struct State {
                int extended_dynamic_state;
                int extended_dynamic_state_2;
                int extended_dynamic_state_2_extra;
                int extended_dynamic_state_3_blend;
                int extended_dynamic_state_3_enables;
                int dynamic_vertex_input;
        };
        std::array<surface::PixelFormat, 8> color_formats;
        surface::PixelFormat depth_format;
        u32 alpha_test_ref;
        u32 point_size;
        int depth_enabled;
        MsaaMode msaa_mode;
        int xfb_state;
        State state;
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

}  // namespace render

namespace std {

template <>
struct hash<render::FixedPipelineState> {
        auto operator()(const render::FixedPipelineState& k) const noexcept -> size_t {
            return k.Hash();
        }
};

}  // namespace std
namespace render {
enum class IndexFormat : u32 {
    UnsignedByte = 0x0,
    UnsignedShort = 0x1,
    UnsignedInt = 0x2,
};

struct PipelineState {
        bool colorBlendEnable = true;
        bool logicOpEnable = false;
        bool stencilTestEnable = true;
        bool depthClampEnable = true;
        bool depthWriteEnable = true;
        bool depthTestEnable = true;
        bool depthBoundsTestEnable = true;
        bool cullMode = false;
        bool depthBiasEnable = true;
        bool rasterizerDiscardEnable = false;
        bool primitiveRestartEnable = false;
        struct ViewPort {
                f32 x{};
                f32 y{};
                f32 width{};
                f32 height{};
                f32 minDepth{};
                f32 maxDepth{1};
        };
        struct Scissors {
                int32_t x = 0;
                int32_t y = 0;
                int32_t width = 0;
                int32_t height = 0;
        };
        ViewPort viewport;
        Scissors scissors;

        // 主要影响render pass的clear value或者command buffer的clearAttachments
        struct ClearColor {
                float r{};
                float g{};
                float b{};
                float a{};
                float depth{1};
                int stencil{};
        };

        ClearColor clearColor;

        // cmdbuf.setBlendConstants
        struct BlendColor {
                float r{};
                float g{};
                float b{};
                float a{};
        };
        BlendColor blendColor;
};
}  // namespace render