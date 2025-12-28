#pragma once
#include "common/common_funcs.hpp"
#include "common/assert.hpp"
#include "render_core/types.hpp"
#include <span>
#include "render_core/pipeline_state.h"
#include <spdlog/spdlog.h>
#include "resource/id.hpp"
namespace ecs {
class Scene;
}
namespace graphics {

template <typename T>
concept ByteSpanConvertible = requires(const T& t) {
    { t.as_byte_span() } -> std::same_as<std::span<const std::byte>>;
};

auto getModelScene() -> ecs::Scene&;

struct EmptyUnformBuffer {
        [[nodiscard]] auto as_byte_span() const -> std::span<const std::byte> { return {}; }
};

struct EmptyPushConstants {
        [[nodiscard]] auto as_byte_span() const -> std::span<const std::byte> { return {}; }
};

struct MeshMaterialResource {
        render::TextureId ambientTextures{};
        render::TextureId diffuseTextures{};
        render::TextureId specularTextures{};
        render::TextureId normalTextures{};
};


}  // namespace graphics