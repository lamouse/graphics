module;
#include <utility>

#include <cassert>
export module render.texture.sample.helper;
import render.texture.types;

export namespace render::texture {

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

[[nodiscard]] inline auto NumSamples(MsaaMode msaa_mode) -> int {
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

[[nodiscard]] inline auto NumSamplesX(MsaaMode msaa_mode) -> int {
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