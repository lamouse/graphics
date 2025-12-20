// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#ifdef _MSC_VER
#pragma warning(disable : 4201)
#endif

#include <array>
#include <type_traits>
#include "common/bit_field.hpp"
#include "common/common_types.hpp"
#include "render_core/surface.hpp"
#include "pipeline_dynamic_state.hpp"
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
        union {
                u32 raw1;
                BitField<0, 1, u32> extended_dynamic_state;
                BitField<1, 1, u32> extended_dynamic_state_2;
                BitField<2, 1, u32> extended_dynamic_state_2_extra;
                BitField<3, 1, u32> extended_dynamic_state_3_blend;
                BitField<4, 1, u32> extended_dynamic_state_3_enables;
                BitField<5, 1, u32> dynamic_vertex_input;
                BitField<6, 1, u32> xfb_enabled;
                BitField<7, 1, u32> ndc_minus_one_to_one;

                BitField<24, 4, PrimitiveTopology> topology;
                BitField<28, 4, MsaaMode> msaa_mode;
        };

        union {
                u32 raw2;
                BitField<1, 3, u32> alpha_test_func;
                BitField<4, 1, u32> early_z;
                BitField<5, 1, u32> depth_enabled;
                BitField<11, 1, u32> y_negate;
                BitField<12, 1, u32> provoking_vertex_last;
                BitField<13, 1, u32> conservative_raster_enable;
                BitField<14, 1, u32> smooth_lines;
                BitField<15, 1, u32> alpha_to_coverage_enabled;
                BitField<16, 1, u32> alpha_to_one_enabled;
        };
        struct DynamicState {
                union {
                        u32 raw1;
                        BitField<0, 2, u32> cull_face;
                        BitField<2, 1, u32> cull_enable;
                        BitField<3, 1, u32> primitive_restart_enable;
                        BitField<4, 1, u32> depth_bias_enable;
                        BitField<5, 1, u32> rasterize_enable;
                        BitField<6, 4, u32> logic_op;
                        BitField<10, 1, u32> logic_op_enable;
                        BitField<11, 1, u32> depth_clamp_disabled;
                        BitField<12, 1, u32> line_stipple_enable;
                };
                union {
                        u32 raw2;
                        //     StencilFace<0> front;
                        //     StencilFace<12> back;
                        BitField<24, 1, u32> stencil_enable;
                        BitField<25, 1, u32> depth_write_enable;
                        BitField<26, 1, u32> depth_bounds_enable;
                        BitField<27, 1, u32> depth_test_enable;
                        BitField<28, 1, u32> front_face;
                        BitField<29, 3, u32> depth_test_func;
                };

                StencilOp front;
                StencilOp back;
        };

        std::array<surface::PixelFormat, 8> color_formats;
        surface::PixelFormat depth_format;
        u32 point_size;
        DynamicState dynamicState;

        u32 line_stipple_factor;
        u32 line_stipple_pattern;

        u32 depth_bounds_min;
        u32 depth_bounds_max;
        [[nodiscard]] auto Hash() const noexcept -> size_t;

        auto operator==(const FixedPipelineState& rhs) const noexcept -> bool;

        auto operator!=(const FixedPipelineState& rhs) const noexcept -> bool {
            return !operator==(rhs);
        }

        [[nodiscard]] auto Size() const noexcept -> size_t { return sizeof(*this); }

        void Refresh(DynamicFeatures& features);
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

struct DynamicPipelineState {
        union {
                struct {
                        uint32_t colorBlendEnable : 1;
                        uint32_t stencilTestEnable : 1;
                        uint32_t depthClampEnable : 1;
                        uint32_t depthWriteEnable : 1;
                        uint32_t depthTestEnable : 1;
                        uint32_t depthBoundsTestEnable : 1;
                        uint32_t cullMode : 1;
                        uint32_t depthBiasEnable : 1;
                        uint32_t rasterizerDiscardEnable : 1;
                        uint32_t primitiveRestartEnable : 1;
                        uint32_t stencil_two_side_enable : 1;
                        uint32_t reserved : 21;
                };
                uint32_t flags{};
        };
        struct ViewPort {
                f32 x{};
                f32 y{};
                f32 width{};
                f32 height{};
                f32 minDepth{};
                f32 maxDepth{1};

                auto operator<=>(const ViewPort& rhs) const noexcept = default;
        };
        struct Scissors {
                int32_t x = 0;
                int32_t y = 0;
                int32_t width = 0;
                int32_t height = 0;
                auto operator<=>(const Scissors& rhs) const noexcept = default;
        };
        ViewPort viewport;
        Scissors scissors;

        // cmdbuf.setBlendConstants
        struct BlendColor {
                float r{};
                float g{};
                float b{};
                float a{};
                auto operator<=>(const BlendColor& rhs) const noexcept = default;
        };
        BlendColor blendColor;

        struct StencilProperties {
                u32 ref = 0;
                u32 write_mask = 255;
                u32 compare_mask = 255;
                auto operator<=>(const StencilProperties& rhs) const noexcept = default;
        };

        StencilOp frontStencilOp = DEFAULT_STENCIL_OP;
        StencilOp backStencilOp = DEFAULT_STENCIL_OP;
        StencilProperties stencilFrontProperties{};
        StencilProperties stencilBackProperties{};
        ComparisonOp depthComparison{ComparisonOp::LessEqual};
        CullFace cullFace{CullFace::Back};

        // 构造函数：设置默认标志位
        DynamicPipelineState()
            : colorBlendEnable(1),
              stencilTestEnable(1),
              depthClampEnable(1),
              depthWriteEnable(1),
              depthTestEnable(1),
              depthBoundsTestEnable(1),
              cullMode(0),
              depthBiasEnable(1),
              rasterizerDiscardEnable(0),
              primitiveRestartEnable(0),
              stencil_two_side_enable(0),
              reserved(0)
        // viewport, scissors, blendColor 使用 {} 初始化，已为 0
        {}
};
}  // namespace render