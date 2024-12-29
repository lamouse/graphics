#pragma once

#include <cstdint>
namespace render::frame {
enum class BlendMode : uint8_t {
    Opaque,
    Premultiplied,
    Coverage,
};
struct FramebufferConfig {
        BlendMode blending{};
        void* address{};
        uint32_t offset{};
        uint32_t width{};
        uint32_t height{};
        uint32_t stride{};
};
}  // namespace render::frame