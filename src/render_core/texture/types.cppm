module;
#include "render_core/texture/types.hpp"
export module render.texture.types;

export namespace render::texture {

using render::texture::NUM_RT;
using render::texture::MAX_MIP_LEVELS;
using render::texture:: CORRUPT_ID;
using ImageId = render::texture::ImageId;
using ImageMapId = render::texture::ImageMapId;
using ImageViewId = render::texture::ImageViewId;
using SamplerId = render::texture::SamplerId;
using FramebufferId = render::texture::FramebufferId;

/// Fake image ID for null image views
using render::texture:: NULL_IMAGE_ID;
/// Image view ID for null descriptors
using render::texture::NULL_IMAGE_VIEW_ID;
/// Sampler ID for bugged sampler ids
using render::texture::NULL_SAMPLER_ID;


using ImageType = render::texture::ImageType;

using ImageViewType = render::texture::ImageViewType;
using render::texture::NUM_IMAGE_VIEW_TYPES;


using Offset2D = render::texture::Offset2D;

using Offset3D = render::texture::Offset3D;

using Region2D = render::texture::Region2D;
using Extent2D = render::texture::Extent2D;
using Extent3D = render::texture::Extent3D;
using SubresourceLayers = render::texture::SubresourceLayers;
using SubresourceBase = render::texture::SubresourceBase;
using SubresourceExtent = render::texture::SubresourceExtent;
using SubresourceRange = render::texture::SubresourceRange;
using ImageCopy = render::texture::ImageCopy;
using BufferImageCopy = render::texture::BufferImageCopy;
using BufferCopy = render::texture::BufferCopy;
using SwizzleParameters = render::texture::SwizzleParameters;
using MsaaMode = render::texture::MsaaMode;

}  // namespace render::texture

namespace std {
template <>
struct hash<render::texture::Extent3D> {
    auto operator()(const render::texture::Extent3D& ext) const noexcept -> size_t {
        // 使用哈希组合算法（推荐 FNV-1a 风格）
        size_t h1 = std::hash<u32>{}(ext.width);
        size_t h2 = std::hash<u32>{}(ext.height);
        size_t h3 = std::hash<u32>{}(ext.depth);
        return h1 ^ (h2 << 1U) ^ (h3 << 2U);
    }
};
}  // namespace std