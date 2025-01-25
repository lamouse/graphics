#pragma once
#include "render_core/texture_cache/texture_cache_base.hpp"
namespace render::texture {
using surface::GetFormatType;
using surface::PixelFormat;
using surface::SurfaceType;
using namespace common::literals;

template <class P>
TextureCache<P>::TextureCache(Runtime& runtime_) : runtime{runtime_} {}

template <class P>
void TextureCache<P>::WriteMemory(void* data, size_t size) {}

template <class P>
auto TextureCache<P>::InsertImage(const ImageInfo& info, RelaxedOptions options) -> ImageId {
    const ImageId image_id = JoinImages(info);
    const Image& image = slot_images[image_id];
    // Using "image.gpu_addr" instead of "gpu_addr" is important because it might be different
    const auto [it, is_new] = image_allocs_table.try_emplace(image.gpu_addr);
    if (is_new) {
        it->second = slot_image_allocs.insert();
    }
    slot_image_allocs[it->second].images.push_back(image_id);
    return image_id;
}

}  // namespace render::texture