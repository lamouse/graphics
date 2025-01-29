#pragma once
#include "common/common_funcs.hpp"
#include "render_core/texture/image_info.hpp"
#include <span>
namespace render {
class Graphic {
    public:
        virtual ~Graphic() = default;

        virtual void addTexture(const texture::ImageInfo& imageInfo) = 0;
        virtual void addVertex(std::span<float> vertex, const ::std::span<uint16_t>& indices) = 0;
        virtual void drawIndics(u32 indicesSize) = 0;
        virtual void addUniformBuffer(void* data, size_t size) = 0;
        virtual void start() = 0;
        virtual void end() = 0;
        Graphic() = default;
        CLASS_DEFAULT_COPYABLE(Graphic);
        CLASS_DEFAULT_MOVEABLE(Graphic);
};
}  // namespace render