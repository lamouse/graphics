// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <string>

#include <ranges>
#include "formatter.h"
#include "image_base.hpp"
#include "image_info.hpp"
#include "image_view_base.hpp"
#include "render_targets.h"
#include "samples_helper.h"

namespace render::texture {

std::string Name(const ImageBase& image) {
    const ImageInfo& info = image.info;
    u32 width = info.size.width;
    u32 height = info.size.height;
    const u32 depth = info.size.depth;
    const u32 num_layers = image.info.resources.layers;
    const u32 num_levels = image.info.resources.levels;
    std::string resource;
    if (image.info.num_samples > 1) {
        const auto [samples_x, samples_y] = render::texture::SamplesLog2(image.info.num_samples);
        width >>= samples_x;
        height >>= samples_y;
        resource += fmt::format(":{}xMSAA", image.info.num_samples);
    }
    if (num_layers > 1) {
        resource += fmt::format(":L{}", num_layers);
    }
    if (num_levels > 1) {
        resource += fmt::format(":M{}", num_levels);
    }
    switch (image.info.type) {
        case ImageType::e1D:
            return fmt::format("Image 1D  {}{}", width, resource);
        case ImageType::e2D:
            return fmt::format("Image 2D  {}x{}{}", width, height, resource);
        case ImageType::e3D:
            return fmt::format("Image 2D  {}x{}x{}{}", width, height, depth, resource);
        case ImageType::Linear:
            return fmt::format("Image Linear  {}x{}", width, height);
        case ImageType::Buffer:
            return fmt::format("Buffer {}", image.info.size.width);
    }
    return "Invalid";
}

std::string Name(const ImageViewBase& image_view) {
    const u32 width = image_view.size.width;
    const u32 height = image_view.size.height;
    const u32 depth = image_view.size.depth;
    const u32 num_levels = image_view.range.extent.levels;
    const u32 num_layers = image_view.range.extent.layers;

    const std::string level = num_levels > 1 ? fmt::format(":{}", num_levels) : "";
    switch (image_view.type) {
        case ImageViewType::e1D:
            return fmt::format("ImageView 1D  {}{}", width, level);
        case ImageViewType::e2D:
            return fmt::format("ImageView 2D  {}x{}{}", width, height, level);
        case ImageViewType::Cube:
            return fmt::format("ImageView Cube  {}x{}{}", width, height, level);
        case ImageViewType::e3D:
            return fmt::format("ImageView 3D  {}x{}x{}{}", width, height, depth, level);
        case ImageViewType::e1DArray:
            return fmt::format("ImageView 1DArray  {}{}|{}", width, level, num_layers);
        case ImageViewType::e2DArray:
            return fmt::format("ImageView 2DArray {}x{}{}|{}", width, height, level, num_layers);
        case ImageViewType::CubeArray:
            return fmt::format("ImageView CubeArray  {}x{}{}|{}", width, height, level, num_layers);
        case ImageViewType::Rect:
            return fmt::format("ImageView Rect  {}x{}{}", width, height, level);
        case ImageViewType::Buffer:
            return fmt::format("BufferView  {}", width);
    }
    return "Invalid";
}

std::string Name(const RenderTargets& render_targets) {
    std::string_view debug_prefix;
    const auto num_color = std::ranges::count_if(
        render_targets.color_buffer_ids, [](ImageViewId id) { return static_cast<bool>(id); });
    if (render_targets.depth_buffer_id) {
        debug_prefix = num_color > 0 ? "R" : "Z";
    } else {
        debug_prefix = num_color > 0 ? "C" : "X";
    }
    const Extent2D size = render_targets.size;
    if (num_color > 0) {
        return fmt::format("Framebuffer {}{} {}x{}", debug_prefix, num_color, size.width,
                           size.height);
    } else {
        return fmt::format("Framebuffer {} {}x{}", debug_prefix, size.width, size.height);
    }
}

}  // namespace render::texture
