//
// Created by ziyu on 2025/9/20.
//
#include "render_core/render_vulkan/format_to_vk.hpp"
#include "common/assert.hpp"
#include <format>
#include <spdlog/spdlog.h>
import render.vulkan.common;
namespace render::vulkan {
auto VertexFormat(const Device& device, VertexAttribute::Type type, VertexAttribute::Size size)
    -> vk::Format {
    if (device.mustEmulateScaledFormats()) {
        if (type == VertexAttribute::Type::SScaled) {
            type = VertexAttribute::Type::SInt;
        } else if (type == VertexAttribute::Type::UScaled) {
            type = VertexAttribute::Type::UInt;
        }
    }
    const vk::Format format{([&]() {
        switch (type) {
            case VertexAttribute::Type::UnusedEnumDoNotUseBecauseItWillGoAway:
                ASSERT_MSG(false, "Invalid vertex attribute type!");
                break;
            case VertexAttribute::Type::UNorm:
                switch (size) {
                    case VertexAttribute::Size::R8:
                    case VertexAttribute::Size::A8:
                        return VK_FORMAT_R8_UNORM;
                    case VertexAttribute::Size::R8_G8:
                    case VertexAttribute::Size::G8_R8:
                        return VK_FORMAT_R8G8_UNORM;
                    case VertexAttribute::Size::R8_G8_B8:
                        return VK_FORMAT_R8G8B8_UNORM;
                    case VertexAttribute::Size::R8_G8_B8_A8:
                    case VertexAttribute::Size::X8_B8_G8_R8:
                        return VK_FORMAT_R8G8B8A8_UNORM;
                    case VertexAttribute::Size::R16:
                        return VK_FORMAT_R16_UNORM;
                    case VertexAttribute::Size::R16_G16:
                        return VK_FORMAT_R16G16_UNORM;
                    case VertexAttribute::Size::R16_G16_B16:
                        return VK_FORMAT_R16G16B16_UNORM;
                    case VertexAttribute::Size::R16_G16_B16_A16:
                        return VK_FORMAT_R16G16B16A16_UNORM;
                    case VertexAttribute::Size::A2_B10_G10_R10:
                        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
                    default:
                        break;
                }
                break;
            case VertexAttribute::Type::SNorm:
                switch (size) {
                    case VertexAttribute::Size::R8:
                    case VertexAttribute::Size::A8:
                        return VK_FORMAT_R8_SNORM;
                    case VertexAttribute::Size::R8_G8:
                    case VertexAttribute::Size::G8_R8:
                        return VK_FORMAT_R8G8_SNORM;
                    case VertexAttribute::Size::R8_G8_B8:
                        return VK_FORMAT_R8G8B8_SNORM;
                    case VertexAttribute::Size::R8_G8_B8_A8:
                    case VertexAttribute::Size::X8_B8_G8_R8:
                        return VK_FORMAT_R8G8B8A8_SNORM;
                    case VertexAttribute::Size::R16:
                        return VK_FORMAT_R16_SNORM;
                    case VertexAttribute::Size::R16_G16:
                        return VK_FORMAT_R16G16_SNORM;
                    case VertexAttribute::Size::R16_G16_B16:
                        return VK_FORMAT_R16G16B16_SNORM;
                    case VertexAttribute::Size::R16_G16_B16_A16:
                        return VK_FORMAT_R16G16B16A16_SNORM;
                    case VertexAttribute::Size::A2_B10_G10_R10:
                        return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
                    default:
                        break;
                }
                break;
            case VertexAttribute::Type::UScaled:
                switch (size) {
                    case VertexAttribute::Size::R8:
                    case VertexAttribute::Size::A8:
                        return VK_FORMAT_R8_USCALED;
                    case VertexAttribute::Size::R8_G8:
                    case VertexAttribute::Size::G8_R8:
                        return VK_FORMAT_R8G8_USCALED;
                    case VertexAttribute::Size::R8_G8_B8:
                        return VK_FORMAT_R8G8B8_USCALED;
                    case VertexAttribute::Size::R8_G8_B8_A8:
                    case VertexAttribute::Size::X8_B8_G8_R8:
                        return VK_FORMAT_R8G8B8A8_USCALED;
                    case VertexAttribute::Size::R16:
                        return VK_FORMAT_R16_USCALED;
                    case VertexAttribute::Size::R16_G16:
                        return VK_FORMAT_R16G16_USCALED;
                    case VertexAttribute::Size::R16_G16_B16:
                        return VK_FORMAT_R16G16B16_USCALED;
                    case VertexAttribute::Size::R16_G16_B16_A16:
                        return VK_FORMAT_R16G16B16A16_USCALED;
                    case VertexAttribute::Size::A2_B10_G10_R10:
                        return VK_FORMAT_A2B10G10R10_USCALED_PACK32;
                    default:
                        break;
                }
                break;
            case VertexAttribute::Type::SScaled:
                switch (size) {
                    case VertexAttribute::Size::R8:
                    case VertexAttribute::Size::A8:
                        return VK_FORMAT_R8_SSCALED;
                    case VertexAttribute::Size::R8_G8:
                    case VertexAttribute::Size::G8_R8:
                        return VK_FORMAT_R8G8_SSCALED;
                    case VertexAttribute::Size::R8_G8_B8:
                        return VK_FORMAT_R8G8B8_SSCALED;
                    case VertexAttribute::Size::R8_G8_B8_A8:
                    case VertexAttribute::Size::X8_B8_G8_R8:
                        return VK_FORMAT_R8G8B8A8_SSCALED;
                    case VertexAttribute::Size::R16:
                        return VK_FORMAT_R16_SSCALED;
                    case VertexAttribute::Size::R16_G16:
                        return VK_FORMAT_R16G16_SSCALED;
                    case VertexAttribute::Size::R16_G16_B16:
                        return VK_FORMAT_R16G16B16_SSCALED;
                    case VertexAttribute::Size::R16_G16_B16_A16:
                        return VK_FORMAT_R16G16B16A16_SSCALED;
                    case VertexAttribute::Size::A2_B10_G10_R10:
                        return VK_FORMAT_A2B10G10R10_SSCALED_PACK32;
                    default:
                        break;
                }
                break;
            case VertexAttribute::Type::UInt:
                switch (size) {
                    case VertexAttribute::Size::R8:
                    case VertexAttribute::Size::A8:
                        return VK_FORMAT_R8_UINT;
                    case VertexAttribute::Size::R8_G8:
                    case VertexAttribute::Size::G8_R8:
                        return VK_FORMAT_R8G8_UINT;
                    case VertexAttribute::Size::R8_G8_B8:
                        return VK_FORMAT_R8G8B8_UINT;
                    case VertexAttribute::Size::R8_G8_B8_A8:
                    case VertexAttribute::Size::X8_B8_G8_R8:
                        return VK_FORMAT_R8G8B8A8_UINT;
                    case VertexAttribute::Size::R16:
                        return VK_FORMAT_R16_UINT;
                    case VertexAttribute::Size::R16_G16:
                        return VK_FORMAT_R16G16_UINT;
                    case VertexAttribute::Size::R16_G16_B16:
                        return VK_FORMAT_R16G16B16_UINT;
                    case VertexAttribute::Size::R16_G16_B16_A16:
                        return VK_FORMAT_R16G16B16A16_UINT;
                    case VertexAttribute::Size::R32:
                        return VK_FORMAT_R32_UINT;
                    case VertexAttribute::Size::R32_G32:
                        return VK_FORMAT_R32G32_UINT;
                    case VertexAttribute::Size::R32_G32_B32:
                        return VK_FORMAT_R32G32B32_UINT;
                    case VertexAttribute::Size::R32_G32_B32_A32:
                        return VK_FORMAT_R32G32B32A32_UINT;
                    case VertexAttribute::Size::A2_B10_G10_R10:
                        return VK_FORMAT_A2B10G10R10_UINT_PACK32;
                    default:
                        break;
                }
                break;
            case VertexAttribute::Type::SInt:
                switch (size) {
                    case VertexAttribute::Size::R8:
                    case VertexAttribute::Size::A8:
                        return VK_FORMAT_R8_SINT;
                    case VertexAttribute::Size::R8_G8:
                    case VertexAttribute::Size::G8_R8:
                        return VK_FORMAT_R8G8_SINT;
                    case VertexAttribute::Size::R8_G8_B8:
                        return VK_FORMAT_R8G8B8_SINT;
                    case VertexAttribute::Size::R8_G8_B8_A8:
                    case VertexAttribute::Size::X8_B8_G8_R8:
                        return VK_FORMAT_R8G8B8A8_SINT;
                    case VertexAttribute::Size::R16:
                        return VK_FORMAT_R16_SINT;
                    case VertexAttribute::Size::R16_G16:
                        return VK_FORMAT_R16G16_SINT;
                    case VertexAttribute::Size::R16_G16_B16:
                        return VK_FORMAT_R16G16B16_SINT;
                    case VertexAttribute::Size::R16_G16_B16_A16:
                        return VK_FORMAT_R16G16B16A16_SINT;
                    case VertexAttribute::Size::R32:
                        return VK_FORMAT_R32_SINT;
                    case VertexAttribute::Size::R32_G32:
                        return VK_FORMAT_R32G32_SINT;
                    case VertexAttribute::Size::R32_G32_B32:
                        return VK_FORMAT_R32G32B32_SINT;
                    case VertexAttribute::Size::R32_G32_B32_A32:
                        return VK_FORMAT_R32G32B32A32_SINT;
                    case VertexAttribute::Size::A2_B10_G10_R10:
                        return VK_FORMAT_A2B10G10R10_SINT_PACK32;
                    default:
                        break;
                }
                break;
            case VertexAttribute::Type::Float:
                switch (size) {
                    case VertexAttribute::Size::R16:
                        return VK_FORMAT_R16_SFLOAT;
                    case VertexAttribute::Size::R16_G16:
                        return VK_FORMAT_R16G16_SFLOAT;
                    case VertexAttribute::Size::R16_G16_B16:
                        return VK_FORMAT_R16G16B16_SFLOAT;
                    case VertexAttribute::Size::R16_G16_B16_A16:
                        return VK_FORMAT_R16G16B16A16_SFLOAT;
                    case VertexAttribute::Size::R32:
                        return VK_FORMAT_R32_SFLOAT;
                    case VertexAttribute::Size::R32_G32:
                        return VK_FORMAT_R32G32_SFLOAT;
                    case VertexAttribute::Size::R32_G32_B32:
                        return VK_FORMAT_R32G32B32_SFLOAT;
                    case VertexAttribute::Size::R32_G32_B32_A32:
                        return VK_FORMAT_R32G32B32A32_SFLOAT;
                    case VertexAttribute::Size::B10_G11_R11:
                        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
                    default:
                        break;
                }
                break;
        }
        return VK_FORMAT_UNDEFINED;
    })()};
    if (format == vk::Format::eUndefined) {
        UNIMPLEMENTED_MSG(std::format("Unimplemented vertex format of type={} and size={}",
                                      static_cast<u32>(type), static_cast<u32>(size)));
    }
    return device.getSupportedFormat(format, vk::FormatFeatureFlagBits::eVertexBuffer,
                                     FormatType::Buffer);
}

auto PrimitiveTopologyToVK(render::PrimitiveTopology topology) -> vk::PrimitiveTopology {
    switch (topology) {
        case render::PrimitiveTopology::Points:
            return vk::PrimitiveTopology::ePointList;
        case render::PrimitiveTopology::Lines:
            return vk::PrimitiveTopology::eLineList;
        case render::PrimitiveTopology::LineStrip:
            return vk::PrimitiveTopology::eLineStrip;
        case render::PrimitiveTopology::Triangles:
            return vk::PrimitiveTopology::eTriangleList;
        case render::PrimitiveTopology::TriangleStrip:
            return vk::PrimitiveTopology::eTriangleStrip;
        case render::PrimitiveTopology::TriangleFan:
            return vk::PrimitiveTopology::eTriangleFan;
        case render::PrimitiveTopology::LinesAdjacency:
            return vk::PrimitiveTopology::eLineListWithAdjacency;
        case render::PrimitiveTopology::LineStripAdjacency:
            return vk::PrimitiveTopology::eLineStripWithAdjacency;
        case render::PrimitiveTopology::TrianglesAdjacency:
            return vk::PrimitiveTopology::eTriangleListWithAdjacency;
        case render::PrimitiveTopology::TriangleStripAdjacency:
            return vk::PrimitiveTopology::eTriangleStripWithAdjacency;

        case render::PrimitiveTopology::Patches:
            return vk::PrimitiveTopology::ePatchList;
    }
    SPDLOG_ERROR("Unimplemented topology={}", static_cast<int>(topology));
    return {};
}

auto StencilOp(render::StencilOp::Op stencilOp) -> vk::StencilOp {
    using Op = render::StencilOp::Op;

    switch (stencilOp) {
        case Op::Keep:
            return vk::StencilOp::eKeep;
        case Op::Zero:
            return vk::StencilOp::eZero;
        case Op::Replace:
            return vk::StencilOp::eReplace;
        case Op::IncrementAndClamp:
            return vk::StencilOp::eIncrementAndClamp;
        case Op::DecrementAndClamp:
            return vk::StencilOp::eDecrementAndClamp;
        case Op::Invert:
            return vk::StencilOp::eInvert;
        case Op::IncrementAndWrap:
            return vk::StencilOp::eIncrementAndWrap;
        case Op::DecrementAndWrap:
            return vk::StencilOp::eDecrementAndWrap;
    }
    spdlog::warn("Unimplemented stencil op={}", static_cast<int>(stencilOp));
    return {};
}

auto ComparisonOp(render::ComparisonOp compreOp) -> vk::CompareOp {
    switch (compreOp) {
        case render::ComparisonOp::Never:
            return ::vk::CompareOp::eNever;
        case render::ComparisonOp::Less:
            return ::vk::CompareOp::eLess;
        case render::ComparisonOp::Equal:
            return ::vk::CompareOp::eEqual;
        case render::ComparisonOp::LessEqual:
            return ::vk::CompareOp::eLessOrEqual;
        case render::ComparisonOp::Greater:
            return ::vk::CompareOp::eGreater;
        case render::ComparisonOp::NotEqual:
            return ::vk::CompareOp::eNotEqual;
        case render::ComparisonOp::GreaterEqual:
            return ::vk::CompareOp::eGreaterOrEqual;
        case render::ComparisonOp::Always:
            return ::vk::CompareOp::eAlways;
    }

    spdlog::warn("Unimplemented comparison op={}", static_cast<int>(compreOp));
    return {};
}

auto CullFace(render::CullFace cull_face) -> vk::CullModeFlagBits {
    switch (cull_face) {
        case render::CullFace::Front:
            return vk::CullModeFlagBits::eFront;
        case render::CullFace::Back:
            return vk::CullModeFlagBits::eBack;
        case render::CullFace::FrontAndBack:
            return vk::CullModeFlagBits::eFrontAndBack;
    }
    spdlog::warn("Unimplemented cull face={}", static_cast<int>(cull_face));
    return {};
}

}  // namespace render::vulkan