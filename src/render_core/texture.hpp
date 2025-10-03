#pragma once
#include <cstdint>
namespace render {

enum class SwizzleSource :  std::uint8_t {
    Zero = 0,

    R = 2,
    G = 3,
    B = 4,
    A = 5,
    OneInt = 6,
    OneFloat = 7,
};

enum class SamplerReduction : std::uint8_t {
    WeightedAverage = 0,
    Min = 1,
    Max = 2,
};

enum class SamplerPreset : std::uint8_t{
    Linear,
    Nearest,
    ShadowMin,
    HDRMax,
};

}  // namespace render