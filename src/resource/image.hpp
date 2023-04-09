#pragma once
#include <string>
#include <string_view>

namespace resource::image {

    struct ImageInfo{
        int width;
        int height;
        int channels;
    };

    class Image
    {
    private:
        ImageInfo imageInfo;
        unsigned char* data;
    public:
        void readImage(::std::string& path);
        Image(::std::string& path);
        unsigned char* getData();
        ImageInfo getImageInfo();
        uint32_t getMipLevels();
        ~Image();
    };

}