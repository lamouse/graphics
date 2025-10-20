//
// Created by ziyu on 2025/9/20.
//
#pragma once

#ifndef GRAPHICS_FORMAT_TO_VK_HPP
#define GRAPHICS_FORMAT_TO_VK_HPP
#include "render_core/vertex.hpp"
#include "render_core/fixed_pipeline_state.h"
#include <vulkan/vulkan.hpp>

namespace render::vulkan {
class Device;
auto VertexFormat(const Device& device, VertexAttribute::Type type, VertexAttribute::Size size)
    -> vk::Format;

auto PrimitiveTopologyToVK(render::PrimitiveTopology topology) -> vk::PrimitiveTopology;

}  // namespace render::vulkan
#endif  // GRAPHICS_FORMAT_TO_VK_HPP
