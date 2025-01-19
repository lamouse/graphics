// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <type_traits>

#include "common/bit_field.hpp"
#include "common/common_types.hpp"

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
        struct BlendingAttachment {
                union {
                        u32 raw;
                        BitField<0, 1, u32> mask_r;
                        BitField<1, 1, u32> mask_g;
                        BitField<2, 1, u32> mask_b;
                        BitField<3, 1, u32> mask_a;
                        BitField<4, 3, u32> equation_rgb;
                        BitField<7, 3, u32> equation_a;
                        BitField<10, 5, u32> factor_source_rgb;
                        BitField<15, 5, u32> factor_dest_rgb;
                        BitField<20, 5, u32> factor_source_a;
                        BitField<25, 5, u32> factor_dest_a;
                        BitField<30, 1, u32> enable;
                };

                [[nodiscard]] auto Mask() const noexcept -> std::array<bool, 4> {
                    return {mask_r != 0, mask_g != 0, mask_b != 0, mask_a != 0};
                }
        };

        union VertexAttribute {
                u32 raw;
                BitField<0, 1, u32> enabled;
                BitField<1, 5, u32> buffer;
                BitField<6, 14, u32> offset;
                BitField<20, 3, u32> type;
                BitField<23, 6, u32> size;
        };

        template <size_t Position>
        union StencilFace {
                BitField<Position + 0, 3, u32> action_stencil_fail;
                BitField<Position + 3, 3, u32> action_depth_fail;
                BitField<Position + 6, 3, u32> action_depth_pass;
                BitField<Position + 9, 3, u32> test_func;
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
                };
                union {
                        u32 raw2;
                        StencilFace<0> front;
                        StencilFace<12> back;
                        BitField<24, 1, u32> stencil_enable;
                        BitField<25, 1, u32> depth_write_enable;
                        BitField<26, 1, u32> depth_bounds_enable;
                        BitField<27, 1, u32> depth_test_enable;
                        BitField<28, 1, u32> front_face;
                        BitField<29, 3, u32> depth_test_func;
                };
        };

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
                BitField<8, 2, u32> polygon_mode;
                BitField<10, 2, u32> tessellation_primitive;
                BitField<12, 2, u32> tessellation_spacing;
                BitField<14, 1, u32> tessellation_clockwise;
                BitField<15, 5, u32> patch_control_points_minus_one;

                BitField<24, 4, PrimitiveTopology> topology;
                BitField<28, 4, MsaaMode> msaa_mode;
        };
        union {
                u32 raw2;
                BitField<1, 3, u32> alpha_test_func;
                BitField<4, 1, u32> early_z;
                BitField<5, 1, u32> depth_enabled;
                BitField<6, 5, u32> depth_format;
                BitField<11, 1, u32> y_negate;
                BitField<12, 1, u32> provoking_vertex_last;
                BitField<13, 1, u32> conservative_raster_enable;
                BitField<14, 1, u32> smooth_lines;
                BitField<15, 1, u32> alpha_to_coverage_enabled;
                BitField<16, 1, u32> alpha_to_one_enabled;
                BitField<17, 3, EngineHint> app_stage;
        };
        std::array<u8, 8> color_formats;

        u32 alpha_test_ref;
        u32 point_size;
        std::array<u16, 16> viewport_swizzles;
        union {
                u64 attribute_types;  // Used with VK_EXT_vertex_input_dynamic_state
                u64 enabled_divisors;
        };

        DynamicState dynamic_state;
        std::array<BlendingAttachment, 8> attachments;
        std::array<VertexAttribute, 32> attributes;
        std::array<u32, 32> binding_divisors;
        // Vertex stride is a 12 bits value, we have 4 bits to spare per element
        std::array<u16, 32> vertex_strides;

        int xfb_state;
        [[nodiscard]] auto Hash() const noexcept -> size_t;

        auto operator==(const FixedPipelineState& rhs) const noexcept -> bool;

        auto operator!=(const FixedPipelineState& rhs) const noexcept -> bool {
            return !operator==(rhs);
        }

        [[nodiscard]] auto Size() const noexcept -> size_t {
            if (xfb_enabled) {
                // When transform feedback is enabled, use the whole struct
                return sizeof(*this);
            }
            if (dynamic_vertex_input && extended_dynamic_state_3_blend) {
                // Exclude dynamic state and attributes
                return offsetof(FixedPipelineState, dynamic_state);
            }
            if (dynamic_vertex_input) {
                // Exclude dynamic state
                return offsetof(FixedPipelineState, attributes);
            }
            if (extended_dynamic_state) {
                // Exclude dynamic state
                return offsetof(FixedPipelineState, vertex_strides);
            }
            // Default
            return offsetof(FixedPipelineState, xfb_state);
        }

        [[nodiscard]] auto DynamicAttributeType(size_t index) const noexcept -> u32 {
            return (attribute_types >> (index * 2)) & 0b11;
        }
};
// static_assert(std::has_unique_object_representations_v<FixedPipelineState>);
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
