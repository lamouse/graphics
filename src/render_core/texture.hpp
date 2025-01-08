#pragma once
#include "common/common_types.hpp"
namespace render {

enum class SwizzleSource : u32 {
    Zero = 0,

    R = 2,
    G = 3,
    B = 4,
    A = 5,
    OneInt = 6,
    OneFloat = 7,
};

enum class SamplerReduction : u32 {
    WeightedAverage = 0,
    Min = 1,
    Max = 2,
};

}  // namespace render