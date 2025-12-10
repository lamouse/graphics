#include "accelerated_swizzle.h"
namespace render::texture::Accelerated {
[[nodiscard]] BlockLinearSwizzle2DParams MakeBlockLinearSwizzle2DParams(
    const SwizzleParameters& swizzle, const ImageInfo& info) {
    return {};
}

[[nodiscard]] BlockLinearSwizzle3DParams MakeBlockLinearSwizzle3DParams(
    const SwizzleParameters& swizzle, const ImageInfo& info) {
    return {};
}
}  // namespace render::texture::Accelerated