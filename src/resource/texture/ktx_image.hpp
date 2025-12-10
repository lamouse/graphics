#pragma once
#include <string>
#include <ktx.h>
#include "common/common_funcs.hpp"

namespace resource::image {
class KtxImage {
    public:
        CLASS_NON_COPYABLE(KtxImage);
        CLASS_NON_MOVEABLE(KtxImage);
        KtxImage(const std::string& path);
        auto getKtxTexture() { return ktxTexture; }
        ~KtxImage();

    private:
        ktxTexture* ktxTexture = nullptr;
};
}  // namespace resource::image