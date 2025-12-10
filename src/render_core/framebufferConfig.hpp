#pragma once

#include <cstdint>
#include "common/common_types.hpp"
#include "common/math_util.h"

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

auto NormalizeCrop(const FramebufferConfig& framebuffer, u32 texture_width, u32 texture_height)
    -> common::Rectangle<f32>;
}  // namespace render::frame