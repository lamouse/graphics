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
    imageInfo.size.width = width;
    imageInfo.size.height = height;
    imageInfo.type = render::texture::ImageType::e2D;
    imageInfo.data = data_;
    imageInfo.format = render::surface::PixelFormat::A8B8G8R8_UNORM;
    imageInfo.num_samples = 1;
    imageInfo.resources.levels = 1;

    if (!data_) {
        throw ::std::runtime_error("failed to load texture image!");
    }
    map_data = std::span<unsigned char>(data_, size());
}
Image::Image(const ::std::string& path) { readImage(path); }
auto Image::getData() -> unsigned char* { return data_; }
auto Image::getImageInfo() -> render::texture::ImageInfo& { return imageInfo; }

Image::~Image() { stbi_image_free(data_); }

auto Image::getMipLevels() const -> uint32_t {
    return static_cast<uint32_t>(
               ::std::floor(::std::log2(::std::max(imageInfo.size.width, imageInfo.size.height)))) +
           1;
}

auto Image::size() const -> unsigned long long {
    return static_cast<unsigned long long>(width * height) * 4;
}

}  // namespace resource::image
