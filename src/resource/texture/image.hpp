#pragma once
#include <cstdint>
#include <string>
#include <span>

namespace resource::image {

class ITexture {
    public:
        auto getData() -> unsigned char*;
        [[nodiscard]] virtual auto getWidth() const -> int = 0;
        [[nodiscard]] virtual auto getheight() const -> int = 0;
        [[nodiscard]] virtual auto getMipLevels() const -> uint32_t = 0;
        [[nodiscard]] virtual auto size() const -> unsigned long long = 0;
        [[nodiscard]] virtual auto data() const -> std::span<unsigned char> = 0;
        virtual ~ITexture() = default;
};

class Image :public ITexture{
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
        [[nodiscard]] auto getWidth() const -> int override { return width; }
        [[nodiscard]] auto getheight() const -> int override{ return height; }
        [[nodiscard]] auto getMipLevels() const -> uint32_t override;
        [[nodiscard]] auto size() const -> unsigned long long override;
        auto data() const -> std::span<unsigned char> override { return map_data; }
        ~Image();
};

}  // namespace resource::image