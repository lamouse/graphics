// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>

#include <boost/container/small_vector.hpp>

#include "common/common_types.hpp"
#include "image_base.hpp"

namespace render::texture {
[[nodiscard]] auto CalculateGuestSizeInBytes(const ImageInfo& info) noexcept -> u32;
[[nodiscard]] auto CalculateLayerSize(const ImageInfo& info) noexcept -> u32;
}  // namespace render::texture
