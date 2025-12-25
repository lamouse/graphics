//
// Created by ziyu on 2025/9/20.
//
#pragma once

#ifndef GRAPHICS_FORMAT_TO_VK_HPP
#define GRAPHICS_FORMAT_TO_VK_HPP
#include "render_core/vertex.hpp"
#include "render_core/pipeline_state.h"
#include <vulkan/vulkan.hpp>
import render.vulkan.common;
namespace render::vulkan {

auto VertexFormat(const Device& device, VertexAttribute::Type type, VertexAttribute::Size size)
    -> vk::Format;

auto PrimitiveTopologyToVK(render::PrimitiveTopology topology) -> vk::PrimitiveTopology;

auto StencilOp(render::StencilOp::Op stencilOp) -> vk::StencilOp;

auto ComparisonOp(render::ComparisonOp compreOp) -> vk::CompareOp;

auto CullFace(render::CullFace cull_face) -> vk::CullModeFlagBits;

}  // namespace render::vulkan
#endif  // GRAPHICS_FORMAT_TO_VK_HPP
