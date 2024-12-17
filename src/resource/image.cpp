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

// 进行逆伽马校正
void ConvertSRGBToLinear(unsigned char* data, int width, int height) {
    for (int i = 0; i < width * height * 4; i += 4) {
        // R 通道
        float r = data[i] / 255.0f;
        r = (r <= 0.04045f) ? r / 12.92f : pow((r + 0.055f) / 1.055f, 2.4f);

        // G 通道
        float g = data[i + 1] / 255.0f;
        g = (g <= 0.04045f) ? g / 12.92f : pow((g + 0.055f) / 1.055f, 2.4f);

        // B 通道
        float b = data[i + 2] / 255.0f;
        b = (b <= 0.04045f) ? b / 12.92f : pow((b + 0.055f) / 1.055f, 2.4f);

        // 重新赋值到数据数组
        data[i] = static_cast<unsigned char>(r * 255.0f);
        data[i + 1] = static_cast<unsigned char>(g * 255.0f);
        data[i + 2] = static_cast<unsigned char>(b * 255.0f);
    }
}


void Image::readImage(::std::string& path) {
    data = stbi_load(path.c_str(), &imageInfo.width, &imageInfo.height, &imageInfo.channels, STBI_rgb_alpha);
    if (!data) {
        throw ::std::runtime_error("failed to load texture image!");
    }
    ConvertSRGBToLinear(data, imageInfo.width, imageInfo.height);
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
