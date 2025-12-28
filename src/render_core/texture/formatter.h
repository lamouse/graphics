// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>


namespace render::texture {

struct ImageInfo;
struct ImageViewBase;
struct RenderTargets;

[[nodiscard]] auto Name(const ImageInfo& image) -> std::string;

[[nodiscard]] auto Name(const ImageViewBase& image_view) -> std::string;

[[nodiscard]] auto Name(const RenderTargets& render_targets) -> std::string;

}  // namespace render::texture
