#include "image.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#if _MSC_VER or __APPLE__
#include <stb_image.h>
#else
#include <stb/stb_image.h>
#endif
namespace resource::image {

void Image::readImage(::std::string& path) {
    int channels = 0;
    int w{}, h{};
    data = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    imageInfo.size.width = w;
    imageInfo.size.height = h;
    imageInfo.type = render::texture::ImageType::e2D;
    imageInfo.data = data;
    imageInfo.format = render::surface::PixelFormat::B8G8R8A8_UNORM;
    imageInfo.num_samples = 1;
    imageInfo.resources.levels = 1;
    imageInfo.layer_stride = 4;
    if (!data) {
        throw ::std::runtime_error("failed to load texture image!");
    }
}
Image::Image(::std::string& path) { readImage(path); }
auto Image::getData() -> unsigned char* { return data; }
auto Image::getImageInfo() -> render::texture::ImageInfo& { return imageInfo; }

Image::~Image() { stbi_image_free(data); }

auto Image::getMipLevels() const -> uint32_t {
    return static_cast<uint32_t>(
               ::std::floor(::std::log2(::std::max(imageInfo.size.width, imageInfo.size.height)))) +
           1;
}

auto Image::size() const -> unsigned long long {
    return static_cast<unsigned long long>(imageInfo.size.width * imageInfo.size.height) * 4;
}

}  // namespace resource::image
