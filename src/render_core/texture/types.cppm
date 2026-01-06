module;
#include "render_core/texture/types.hpp"
export module render.texture.types;

import common;

export namespace render::texture {

using render::texture::NUM_RT;
using render::texture::MAX_MIP_LEVELS;
using render::texture:: CORRUPT_ID;
using ImageId = common::SlotId;
using ImageMapId = common::SlotId;
using ImageViewId = common::SlotId;
using SamplerId = common::SlotId;
using FramebufferId = common::SlotId;

/// Fake image ID for null image views
using render::texture:: NULL_IMAGE_ID;
/// Image view ID for null descriptors
using render::texture::NULL_IMAGE_VIEW_ID;
/// Sampler ID for bugged sampler ids
using render::texture::NULL_SAMPLER_ID;


using ImageType = render::texture::ImageType;

using ImageViewType = render::texture::ImageViewType;
// using render::texture::NUM_IMAGE_VIEW_TYPES;


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

export using std::hash;