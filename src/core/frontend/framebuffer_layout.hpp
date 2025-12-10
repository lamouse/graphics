// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.hpp"
#include "common/math_util.h"

namespace layout {

enum class AspectRatio : std::uint8_t {
    Default,
    R4_3,
    R21_9,
    R16_10,
    R32_9,
    StretchToWindow,
};

/// Describes the layout of the window framebuffer
struct FrameBufferLayout {
        u32 width{};
        u32 height{};
        common::Rectangle<u32> screen;
        bool is_srgb{};
};

/**
 * Factory method for constructing a default FramebufferLayout
 * @param width Window framebuffer width in pixels
 * @param height Window framebuffer height in pixels
 * @return Newly created FramebufferLayout object with default screen regions initialized
 */
auto DefaultFrameLayout(u32 width, u32 height) -> FrameBufferLayout;

/**
 * Convenience method to determine emulation aspect ratio
 * @param aspect Represents the index of aspect ratio stored in Settings::values.aspect_ratio
 * @param window_aspect_ratio Current window aspect ratio
 * @return Emulation render window aspect ratio
 */
auto EmulationAspectRatio(AspectRatio aspect, float window_aspect_ratio) -> float;

}  // namespace layout
