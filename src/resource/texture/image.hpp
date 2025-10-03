#pragma once
#include <cstdint>
#include <string>
#include <span>
#include <string_view>

namespace resource::image {

class Image {
    private:
        unsigned char* data_{nullptr};
        std::span<unsigned char> map_data;
        int width{};
        int height{};
        int channels = 0;
    public:
        void readImage(const ::std::string& path);
        explicit Image(const ::std::string& path);
        auto getData() -> unsigned char*;
        [[nodiscard]] auto getWidth() const->int{return width;}
        [[nodiscard]] auto getheight() const->int{return height;}
        [[nodiscard]] auto getMipLevels() const -> uint32_t;
        [[nodiscard]] auto size() const -> unsigned long long;
        auto data() ->std::span<unsigned char> {return map_data;}
        ~Image();
};

}  // namespace resource::image