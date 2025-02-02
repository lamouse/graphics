#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include "render_core/texture/image_base.hpp"

namespace resource::image {

class Image {
    private:
        unsigned char* data;
        render::texture::ImageInfo imageInfo;
    public:
        void readImage(::std::string& path);
        Image(::std::string& path);
        auto getData() -> unsigned char*;
        auto getImageInfo() -> render::texture::ImageInfo&;
        [[nodiscard]] auto getMipLevels() const -> uint32_t;
        [[nodiscard]] auto size() const -> unsigned long long;
        ~Image();
};

}  // namespace resource::image