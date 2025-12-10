#include "ktx_image.hpp"
#include <stdexcept>
namespace resource::image {
KtxImage::KtxImage(const std::string& path) {
    KTX_error_code result = ktxTexture_CreateFromNamedFile(
        path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

    if (result != KTX_SUCCESS) {
        throw std::runtime_error("Failed to load KTX2 file");
    }
}
KtxImage::~KtxImage() {
    if (ktxTexture) {
        ktxTexture_Destroy(ktxTexture);
    }
}
}  // namespace resource::image