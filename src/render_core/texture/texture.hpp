#pragma once
#include "common/common_types.hpp"
#include <string>

namespace render {
struct TextureView {
        u32 width;
        u32 height;
        bool isSRGB;
        bool hasAlpha;
        std::string debugName;
};
}  // namespace render