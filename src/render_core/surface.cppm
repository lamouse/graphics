module;
#include "render_core/surface.hpp"
export module render.surface.format;
export namespace render::surface {
using PixelFormat = render::surface::PixelFormat;
using render::surface::IsPixelFormatASTC;
using render::surface::IsPixelFormatSRGB;
using render::surface::IsPixelFormatBCn;
using render::surface::GetFormatType;
using render::surface::SurfaceType;
using render::surface::MaxPixelFormat;
constexpr std::size_t MaxFormat = render::surface::MaxPixelFormat;
}
