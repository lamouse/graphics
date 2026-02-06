#pragma once
#include <string>
#include <ktx.h>
#include <span>

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

/**
 * @brief Create a Cube Map Ktx Image object order right left top bottom front back or
    posx negx posy negy posz negz
 *
 * @param images
 * @param dstDir
 * @return std::string
 */
auto createCubeMapKtxImage(std::span<std::string_view, 6> images, std::string_view name, std::string_view dstDir) -> std::string;
}  // namespace resource::image