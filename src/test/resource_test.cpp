#include "resource/texture/ktx_image.hpp"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
TEST(Resource, createKtxImage) {
    auto image_path = resource::image::createKtxImage("test/test.jpg", "test");
    spdlog::info("createKtxImage path: {}", image_path);
    resource::image::KtxImage ktx_image(image_path);
    auto* ktx = ktx_image.getKtxTexture();
    spdlog::info("image size: {}x{}", ktx->baseWidth, ktx->baseHeight);
    spdlog::info("image mipmap: {}", ktx->numLevels);
    spdlog::info("image layers: {}", ktx->numLayers);
}