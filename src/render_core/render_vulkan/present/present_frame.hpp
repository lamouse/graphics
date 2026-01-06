#pragma once
#include "common/common_types.hpp"
#include <vulkan/vulkan.hpp>
#include "render_core/vulkan_common/vulkan_wrapper.hpp"

namespace render::vulkan::present {

    struct FramebufferTextureInfo {
        vk::Image image;
        vk::ImageView image_view;
        u32 width{};
        u32 height{};
        u32 scaled_width{};
        u32 scaled_height{};
};

struct Frame {
        uint32_t width;
        uint32_t height;
        Image image;
        ImageView image_view;
        vk::CommandBuffer cmdbuf;
        Semaphore render_ready;
        Fence present_done;
};

}