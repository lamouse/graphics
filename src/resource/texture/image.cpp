#include "image.hpp"

#include <cmath>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#if _MSC_VER or __APPLE__
#include <stb_image.h>
#else
#include <stb/stb_image.h>
#endif
namespace resource::image {

void Image::readImage(const ::std::string& path) {
    data_ = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data_) {
        throw ::std::runtime_error("failed to load texture image!");
    }
    map_data = std::span<unsigned char>(data_, size());
}
Image::Image(const ::std::string& path) { readImage(path); }
auto Image::getData() -> unsigned char* { return data_; }

Image::~Image() { stbi_image_free(data_); }

auto Image::getMipLevels() const -> uint32_t {
    return static_cast<uint32_t>(
               ::std::floor(::std::log2(::std::max(width, height)))) +
           1;
}

auto Image::size() const -> unsigned long long {
    return static_cast<unsigned long long>(width * height) * 4;
}

}  // namespace resource::image
