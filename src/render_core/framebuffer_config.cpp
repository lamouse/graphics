#include "framebuffer_config.hpp"
namespace render::frame {
auto NormalizeCrop(const FramebufferConfig& framebuffer, u32 texture_width, u32 texture_height)
    -> common::Rectangle<f32> {
    f32 left{}, top{}, right{}, bottom{};

    if (!framebuffer.crop_rect.IsEmpty()) {
        // If crop rectangle is not empty, apply properties from rectangle.
        left = static_cast<f32>(framebuffer.crop_rect.left);
        top = static_cast<f32>(framebuffer.crop_rect.top);
        right = static_cast<f32>(framebuffer.crop_rect.right);
        bottom = static_cast<f32>(framebuffer.crop_rect.bottom);
    } else {
        // Otherwise, fall back to framebuffer dimensions.
        left = 0;
        top = 0;
        right = static_cast<f32>(framebuffer.width);
        bottom = static_cast<f32>(framebuffer.height);
    }

    // Normalize coordinate space.
    left /= static_cast<f32>(texture_width);
    top /= static_cast<f32>(texture_height);
    right /= static_cast<f32>(texture_width);
    bottom /= static_cast<f32>(texture_height);

    return common::Rectangle<f32>(left, top, right, bottom);
}
}  // namespace render::frame