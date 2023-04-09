#include "image.hpp"

#include <cmath>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
namespace resource::image {

void Image::readImage(::std::string& path)
{
    data = stbi_load(path.c_str(), &imageInfo.width, &imageInfo.height, &imageInfo.channels, STBI_rgb_alpha);
    if (!data) {
        throw ::std::runtime_error("failed to load texture image!");
    }
}
Image::Image(::std::string& path)
{
    readImage(path);
}
unsigned char* Image::getData()
{
    return data;
}
ImageInfo Image::getImageInfo()
{
    return imageInfo;
}

Image::~Image()
{
    stbi_image_free(data);
}

uint32_t Image::getMipLevels()
{
    return static_cast<uint32_t>(::std::floor(::std::log2(::std::max(imageInfo.width, imageInfo.height)))) + 1;
}

}