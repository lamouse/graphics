// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cassert>
#include <cmath>

#include "common/settings.hpp"
#include "core/frontend/framebuffer_layout.hpp"

namespace {
// Finds the largest size subrectangle contained in window area that is confined to the aspect ratio
template <class T>
auto MaxRectangle(common::Rectangle<T> window_area, float screen_aspect_ratio)
    -> common::Rectangle<T> {
    const float scale = std::min(static_cast<float>(window_area.GetWidth()),
                                 static_cast<float>(window_area.GetHeight()) / screen_aspect_ratio);
    return common::Rectangle<T>{0, 0, static_cast<T>(std::round(scale)),
                                static_cast<T>(std::round(scale * screen_aspect_ratio))};
}
}  // namespace

namespace layout {

auto DefaultFrameLayout(u32 width, u32 height) -> FrameBufferLayout {
    assert(width > 0);
    assert(height > 0);
    // The drawing code needs at least somewhat valid values for both screens
    // so just calculate them both even if the other isn't showing.
    FrameBufferLayout res{
        .width = width,
        .height = height,
        .screen = {},
        .is_srgb = false,
    };

    const float window_aspect_ratio = static_cast<float>(height) / static_cast<float>(width);
    const float emulation_aspect_ratio = EmulationAspectRatio(
        static_cast<AspectRatio>(settings::values.aspect_ratio.GetValue()), window_aspect_ratio);

    const common::Rectangle<u32> screen_window_area{0, 0, width, height};
    common::Rectangle<u32> screen = MaxRectangle(screen_window_area, emulation_aspect_ratio);

    if (window_aspect_ratio < emulation_aspect_ratio) {
        screen = screen.TranslateX((screen_window_area.GetWidth() - screen.GetWidth()) / 2);
    } else {
        screen = screen.TranslateY((height - screen.GetHeight()) / 2);
    }

    res.screen = screen;
    return res;
}

auto EmulationAspectRatio(AspectRatio aspect, float window_aspect_ratio) -> float {
    switch (aspect) {
        case AspectRatio::Default:
            return 9.0f / 16.0f;
        case AspectRatio::R4_3:
            return 3.0f / 4.0f;
        case AspectRatio::R21_9:
            return 9.0f / 21.0f;
        case AspectRatio::R16_10:
            return 10.0f / 16.0f;
        case AspectRatio::R32_9:
            return 9.0f / 32.0f;
        case AspectRatio::StretchToWindow:
            return window_aspect_ratio;
        default:
            return 9.0f / 16.0f;
    }
}

}  // namespace layout
