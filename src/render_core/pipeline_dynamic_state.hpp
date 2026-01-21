#pragma once
#include "common/common_types.hpp"
namespace render {

enum class ComparisonOp : u32 {
    Never = 1,
    Less = 2,
    Equal = 3,
    LessEqual = 4,
    Greater = 5,
    NotEqual = 6,
    GreaterEqual = 7,
    Always = 8,
};
struct StencilOp {
        enum class Op : u32 {
            Keep = 0,
            Zero = 1,
            Replace = 2,
            IncrementAndClamp = 3,
            DecrementAndClamp = 4,
            Invert = 5,
            IncrementAndWrap = 6,
            DecrementAndWrap = 7,
        };

        Op fail;
        Op pass;
        Op depthFail;
        ComparisonOp compare;
        auto operator<=>(const StencilOp& rhs) const noexcept = default;
};
enum class CullFace : u32 {
    Front = 1,
    Back = 2,
    FrontAndBack = 3,
};
inline constexpr auto DEFAULT_STENCIL_OP = StencilOp{.fail = StencilOp::Op::Keep,
                                                     .pass = StencilOp::Op::Keep,
                                                     .depthFail = StencilOp::Op::Keep,
                                                     .compare = ComparisonOp::Always};
}  // namespace render