#include "ktx_image.hpp"
#include "resource/texture/image.hpp"
#include <stdexcept>
#include <filesystem>
#include <ranges>
#include "common/file.hpp"

namespace {
void check_ktx_error(KTX_error_code code) {
    if ((code != KTX_SUCCESS)) {
        throw std::runtime_error(ktxErrorString(code));
    }
}
}  // namespace
namespace resource::image {
KtxImage::KtxImage(const std::string& path) {
    check_ktx_error(ktxTexture_CreateFromNamedFile(
        path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &handle));
}
KtxImage::~KtxImage() {
    if (handle) {
        ktxTexture_Destroy(handle);
    }
}

auto createKtxImage(std::string_view path, std::string_view dstDir) -> std::string {
    std::filesystem::path src_path(path);
    Image image(path);
    ktxTextureCreateInfo ci;
    ci.baseWidth = image.getWidth();
    ci.baseHeight = image.getHeight();
    ci.baseDepth = 1;
    ci.numLevels = image.getMipLevels();
    ci.numFaces = 1;
    ci.numLayers = 1;
    ci.isArray = false;
    ci.generateMipmaps = false;
    ci.numDimensions = 2;
    ci.vkFormat = 44;

    ktxTexture2* ktx2 = nullptr;
    check_ktx_error(ktxTexture2_Create(&ci, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktx2));

    check_ktx_error(ktxTexture_SetImageFromMemory(reinterpret_cast<ktxTexture*>(ktx2), 0, 0, 0,
                                                  image.data().data(), image.data().size()));

    std::filesystem::path dst_path(dstDir);

    src_path.replace_extension(".ktx2");
    common::FS::create_dir(dst_path);
    dst_path /= src_path.filename();
    check_ktx_error(
        ktxTexture_WriteToNamedFile(reinterpret_cast<ktxTexture*>(ktx2), dst_path.string().data()));
    return dst_path.string();
}

auto createCubeMapKtxImage(std::span<std::string_view, 6> images, std::string_view name, std::string_view dstDir)
    -> std::string {
    Image image(images[0]);
    ktxTextureCreateInfo ci;
    ci.baseDepth = 1;
    ci.numFaces = 6;
    ci.numLayers = 1;
    ci.isArray = true;
    ci.generateMipmaps = false;
    ci.numDimensions = 2;
    ci.vkFormat = 44;
    ci.baseWidth = image.getWidth();
    ci.baseHeight = image.getHeight();
    ci.numLevels = image.getMipLevels();

    ktxTexture2* ktx2 = nullptr;
    check_ktx_error(ktxTexture2_Create(&ci, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktx2));

    check_ktx_error(ktxTexture_SetImageFromMemory(reinterpret_cast<ktxTexture*>(ktx2), 0, 0, 0,
                                                  image.data().data(), image.data().size()));

    for (const auto& [index, image_path] : std::views::enumerate(images)) {
        if (index == 0) {
            continue;
        }
        Image other_image(image_path);
        check_ktx_error(ktxTexture_SetImageFromMemory(reinterpret_cast<ktxTexture*>(ktx2), 0, 0,
                                                      index, other_image.data().data(),
                                                      other_image.data().size()));
    }
    std::filesystem::path dst_path(dstDir);

    common::FS::create_dir(dst_path);
    dst_path /= (std::string(name) + ".ktx2");
    check_ktx_error(
        ktxTexture_WriteToNamedFile(reinterpret_cast<ktxTexture*>(ktx2), dst_path.string().data()));
    return dst_path.string();
}

}  // namespace resource::image