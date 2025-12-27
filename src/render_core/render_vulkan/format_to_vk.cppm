module;
#include <vulkan/vulkan.hpp>
export module render.vulkan.format_utils;

import render.vulkan.common;
import render;

export namespace render::vulkan {

auto VertexFormat(const Device& device, VertexAttribute::Type type, VertexAttribute::Size size)
    -> vk::Format;

auto PrimitiveTopologyToVK(render::PrimitiveTopology topology) -> vk::PrimitiveTopology;

auto StencilOp(render::StencilOp::Op stencilOp) -> vk::StencilOp;

auto ComparisonOp(render::ComparisonOp compreOp) -> vk::CompareOp;

auto CullFace(render::CullFace cull_face) -> vk::CullModeFlagBits;

}  // namespace render::vulkan
