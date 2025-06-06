#pragma once

#include <cstdint>
#include <array>
#include "common/common_types.hpp"
#include "common/math_util.h"

#ifdef __APPLE__
constexpr u32 NUM_VERTEX_BUFFERS = 16;
#else
constexpr u32 NUM_VERTEX_BUFFERS = 32;
#endif
constexpr u32 NUM_TRANSFORM_FEEDBACK_BUFFERS = 4;
constexpr u32 NUM_GRAPHICS_UNIFORM_BUFFERS = 18;
constexpr u32 NUM_COMPUTE_UNIFORM_BUFFERS = 8;
constexpr u32 NUM_STORAGE_BUFFERS = 16;
constexpr u32 NUM_TEXTURE_BUFFERS = 32;
constexpr u32 NUM_STAGES = 5;
namespace render::frame {
enum class BlendMode : uint8_t {
    Opaque,
    Premultiplied,
    Coverage,
};
struct FramebufferConfig {
        common::Rectangle<int> crop_rect;
        BlendMode blending{};
        uint32_t offset{};
        uint32_t width{};
        uint32_t height{};
        uint32_t stride{};
};

using UniformBufferSizes = std::array<std::array<u32, NUM_GRAPHICS_UNIFORM_BUFFERS>, NUM_STAGES>;
using ComputeUniformBufferSizes = std::array<u32, NUM_COMPUTE_UNIFORM_BUFFERS>;

auto NormalizeCrop(const FramebufferConfig& framebuffer, u32 texture_width, u32 texture_height)
    -> common::Rectangle<f32>;
}  // namespace render::frame