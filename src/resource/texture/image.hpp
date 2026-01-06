#pragma once
#include <cstdint>
#include <string_view>
#include <span>
#include "render_core/mesh.hpp"

namespace resource::image {

class Image : public render::ITexture {
    private:
        unsigned char* data_{nullptr};
        std::span<unsigned char> map_data;
        int width{};
        int height{};
        int channels = 0;

    public:
        void readImage(::std::string_view path);
        explicit Image(::std::string_view path);
        Image(int width_, int height_, std::span<unsigned char> data, std::uint8_t image_count_)
            : map_data(data), width(width_), height(height_) {
            image_count = image_count_;
        }
        auto getData() -> unsigned char*;
        [[nodiscard]] auto getWidth() const -> int override { return width; }
        [[nodiscard]] auto getHeight() const -> int override { return height; }
        [[nodiscard]] auto getMipLevels() const -> uint32_t override;
        [[nodiscard]] auto size() const -> unsigned long long override;
        [[nodiscard]] auto data() const -> std::span<unsigned char> override { return map_data; }
        ~Image();
};

}  // namespace resource::image