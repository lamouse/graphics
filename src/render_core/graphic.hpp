#pragma once
#include "common/common_funcs.hpp"
#include "render_core/texture/image_info.hpp"
namespace render {
class Graphic {
    public:
        virtual ~Graphic() = default;

        virtual void addTexture(const texture::ImageInfo& imageInfo) = 0;
        Graphic() = default;
        CLASS_DEFAULT_COPYABLE(Graphic);
        CLASS_DEFAULT_MOVEABLE(Graphic);
};
}  // namespace render