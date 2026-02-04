#pragma once
#include <string>
#include <ktx.h>

namespace resource::image {
class KtxImage {
    public:
        KtxImage(const KtxImage&) = delete;
        auto operator=(const KtxImage) = delete;
        KtxImage(KtxImage&& image) noexcept : handle(image.handle) {
            image.handle = nullptr;
        }
        auto operator=(KtxImage&& image) noexcept -> KtxImage&{
            this->handle = image.handle;
            image.handle = nullptr;
            return *this;
        }
        KtxImage(const std::string& path);
        auto getKtxTexture() { return handle; }
        ~KtxImage();

    private:
        ktxTexture* handle = nullptr;
};
/**
 * @brief Create a Ktx Image
 *
 * @param path
 * @return std::string ktx
 */
auto createKtxImage(std::string_view sourcePath, std::string_view dstDir) -> std::string;
}  // namespace resource::image