#pragma once
#include "common/common_funcs.hpp"
namespace render {
class Graphic {
    public:
        virtual ~Graphic() = default;
        Graphic() = default;
        CLASS_DEFAULT_COPYABLE(Graphic);
        CLASS_DEFAULT_MOVEABLE(Graphic);
};
}  // namespace render