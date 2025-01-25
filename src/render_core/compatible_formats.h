// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "render_core/surface.hpp"

namespace render::surface {

auto IsViewCompatible(PixelFormat format_a, PixelFormat format_b, bool broken_views,
                      bool native_bgr) -> bool;

auto IsCopyCompatible(PixelFormat format_a, PixelFormat format_b, bool native_bgr) -> bool;

}  // namespace render::surface
