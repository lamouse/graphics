#include "image.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
namespace resource::image {

void Image::readImage(::std::string& path) {
    data = stbi_load(path.c_str(), &imageInfo.width, &imageInfo.height, &imageInfo.channels, STBI_rgb_alpha);
    if (!data) {
        throw ::std::runtime_error("failed to load texture image!");
    }
}
Image::Image(::std::string& path) { readImage(path); }
auto Image::getData() -> unsigned char* { return data; }
auto Image::getImageInfo() -> ImageInfo { return imageInfo; }

Image::~Image() { stbi_image_free(data); }

auto Image::getMipLevels() const -> uint32_t {
    return static_cast<uint32_t>(::std::floor(::std::log2(::std::max(imageInfo.width, imageInfo.height)))) + 1;
}

auto Image::size() const -> unsigned long long {
    return static_cast<unsigned long long>(imageInfo.width * imageInfo.height) * 4;
}

}  // namespace resource::image