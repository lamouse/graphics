module;
#include <vulkan/vulkan.hpp>
export module render.vulkan.present.present_frame;
import render.vulkan.common;
import common.types;

export namespace render::vulkan{
struct Frame {
    uint32_t width;
    uint32_t height;
    Image image;
    ImageView image_view;
    vk::CommandBuffer cmdbuf;
    Semaphore render_ready;
    Fence present_done;
};

struct FramebufferTextureInfo {
    vk::Image image;
    vk::ImageView image_view;
    u32 width{};
    u32 height{};
    u32 scaled_width{};
    u32 scaled_height{};
};
}