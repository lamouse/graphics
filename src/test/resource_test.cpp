#include "resource/texture/ktx_image.hpp"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <array>
#include <filesystem>
#ifndef IMAGE_RESOURCE_PATH
#define IMAGE_RESOURCE_PATH std::string(".")
#endif
TEST(Resource, createKtxImage) {
    spdlog::info("current path{}", std::filesystem::current_path().string());
    auto image_path = resource::image::createKtxImage(
        std::string(IMAGE_RESOURCE_PATH) + "/test/image/test/test.jpg",
        std::string(IMAGE_RESOURCE_PATH) + "/test/image/test");
    spdlog::info("createKtxImage path: {}", image_path);
    resource::image::KtxImage ktx_image(image_path);
    auto* ktx = ktx_image.getKtxTexture();
    spdlog::info("image size: {}x{}", ktx->baseWidth, ktx->baseHeight);
    spdlog::info("image mipmap: {}", ktx->numLevels);
    spdlog::info("image layers: {}", ktx->numLayers);
}

TEST(Resource, createKtxCubeMap) {
    auto right = std::string(IMAGE_RESOURCE_PATH) + "/test/image/test/sky/posx.jpg";
    auto left = std::string(IMAGE_RESOURCE_PATH) + "/test/image/test/sky/negx.jpg";
    auto top = std::string(IMAGE_RESOURCE_PATH) + "/test/image/test/sky/posy.jpg";
    auto bottom = std::string(IMAGE_RESOURCE_PATH) + "/test/image/test/sky/negy.jpg";
    auto front = std::string(IMAGE_RESOURCE_PATH) + "/test/image/test/sky/posz.jpg";
    auto back = std::string(IMAGE_RESOURCE_PATH) + "/test/image/test/sky/negz.jpg";
    std::array<std::string_view, 6> image_paths{
        right,
        left,
        top,
        bottom,
        front,
        back
    };
    auto image_path = resource::image::createCubeMapKtxImage(image_paths, "sky", "test/image/test");
    spdlog::info("createKtxImage path: {}", image_path);
    resource::image::KtxImage ktx_image(image_path);
    auto* ktx = ktx_image.getKtxTexture();
    spdlog::info("cube map size: {}x{}", ktx->baseWidth, ktx->baseHeight);
    spdlog::info("cube map mipmap: {}", ktx->numLevels);
    spdlog::info("cube map layers: {}", ktx->numLayers);
    ASSERT_EQ(6, ktx->numFaces);
    ASSERT_EQ(true, ktx->isCubemap);
}
