#pragma once

#include "common/common_types.hpp"

namespace shader {

enum class Stage : u32 {
    Vertex,
    TessellationControl,
    TessellationEval,
    Geometry,
    Fragment,
    Compute,
};
constexpr u32 MaxStageTypes = 6;

[[nodiscard]] constexpr auto StageFromIndex(std::size_t index) noexcept -> Stage {
    return static_cast<Stage>(static_cast<std::size_t>(Stage::Vertex) + index);
}

}  // namespace shader
