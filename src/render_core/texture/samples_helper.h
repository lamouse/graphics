// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <utility>

#include <cassert>
#include "types.hpp"

namespace render::texture {

[[nodiscard]] inline auto SamplesLog2(int num_samples) -> std::pair<std::uint8_t, std::uint8_t> {
    switch (num_samples) {
        case 1:
            return {0, 0};
        case 2:
            return {1, 0};
        case 4:
            return {1, 1};
        case 8:
            return {2, 1};
        case 16:
            return {2, 2};
        default:
            assert(false && "Invalid number of samples");
            return {0, 0};
    }
}

[[nodiscard]] inline int NumSamples(MsaaMode msaa_mode) {
    switch (msaa_mode) {
        case MsaaMode::Msaa1x1:
            return 1;
        case MsaaMode::Msaa2x1:
        case MsaaMode::Msaa2x1_D3D:
            return 2;
        case MsaaMode::Msaa2x2:
        case MsaaMode::Msaa2x2_VC4:
        case MsaaMode::Msaa2x2_VC12:
            return 4;
        case MsaaMode::Msaa4x2:
        case MsaaMode::Msaa4x2_D3D:
        case MsaaMode::Msaa4x2_VC8:
        case MsaaMode::Msaa4x2_VC24:
            return 8;
        case MsaaMode::Msaa4x4:
            return 16;
    }
    assert(false && "Invalid MSAA mode=");
    return 1;
}

[[nodiscard]] inline int NumSamplesX(MsaaMode msaa_mode) {
    switch (msaa_mode) {
        case MsaaMode::Msaa1x1:
            return 1;
        case MsaaMode::Msaa2x1:
        case MsaaMode::Msaa2x1_D3D:
        case MsaaMode::Msaa2x2:
        case MsaaMode::Msaa2x2_VC4:
        case MsaaMode::Msaa2x2_VC12:
            return 2;
        case MsaaMode::Msaa4x2:
        case MsaaMode::Msaa4x2_D3D:
        case MsaaMode::Msaa4x2_VC8:
        case MsaaMode::Msaa4x2_VC24:
        case MsaaMode::Msaa4x4:
            return 4;
    }
    assert(false && "Invalid MSAA mode={}");
    return 1;
}

[[nodiscard]] inline auto NumSamplesY(MsaaMode msaa_mode) -> int {
    switch (msaa_mode) {
        case MsaaMode::Msaa1x1:
        case MsaaMode::Msaa2x1:
        case MsaaMode::Msaa2x1_D3D:
            return 1;
        case MsaaMode::Msaa2x2:
        case MsaaMode::Msaa2x2_VC4:
        case MsaaMode::Msaa2x2_VC12:
        case MsaaMode::Msaa4x2:
        case MsaaMode::Msaa4x2_D3D:
        case MsaaMode::Msaa4x2_VC8:
        case MsaaMode::Msaa4x2_VC24:
            return 2;
        case MsaaMode::Msaa4x4:
            return 4;
    }
    assert(false && "Invalid MSAA mode={}");
    return 1;
}

}  // namespace render::texture
