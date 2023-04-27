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
        auto getData() -> unsigned char*;
        auto getImageInfo() -> ImageInfo;
        [[nodiscard]] auto getMipLevels()const -> uint32_t;
        [[nodiscard]] auto size()const -> unsigned long long;
        ~Image();
    };

}