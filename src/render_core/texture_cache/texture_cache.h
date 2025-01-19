#pragma once
#include "render_core/texture_cache/texture_cache_base.hpp"
namespace render::texture {
using surface::GetFormatType;
using surface::PixelFormat;
using surface::SurfaceType;
using namespace common::literals;

template <class P>
TextureCache<P>::TextureCache(Runtime& runtime_) : runtime{runtime_} {}

}  // namespace render::texture