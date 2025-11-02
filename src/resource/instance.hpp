#pragma once
#include "common/common_funcs.hpp"
#include "common/assert.hpp"
#include "render_core/types.hpp"
#include <span>
#include "render_core/pipeline_state.h"
#include "ecs/scene/entity.hpp"
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

class IModelInstance {
    public:
        IModelInstance() = default;
        virtual ~IModelInstance() = default;
        CLASS_DEFAULT_COPYABLE(IModelInstance);
        CLASS_DEFAULT_MOVEABLE(IModelInstance);
        [[nodiscard]] auto getTextureId() const -> render::TextureId { return textureId; };
        [[nodiscard]] auto getMeshId() const -> render::MeshId { return meshId; };
        [[nodiscard]] auto vertexShaderHash() const -> std::uint64_t {
            ASSERT_MSG(vertex_shader_hash, "vertex_shader_hash can't be 0");
            return vertex_shader_hash;
        }
        [[nodiscard]] auto fragmentShaderHash() const -> std::uint64_t {
            ASSERT_MSG(fragment_shader_hash, "fragment_shader_hash can't be 0");
            return fragment_shader_hash;
        }
        [[nodiscard]] auto getVertexCount() const -> std::int32_t { return vertex_count; }
        [[nodiscard]] void setVertexCount(std::int32_t count) { vertex_count = count; }
        [[nodiscard]] virtual auto getUBOData() const -> std::span<const std::byte> = 0;
        [[nodiscard]] virtual auto getPushConstants() const -> std::span<const std::byte> = 0;
        [[nodiscard]] virtual auto getPrimitiveTopology() const -> render::PrimitiveTopology = 0;
        [[nodiscard]] virtual auto getPipelineState() const -> render::DynamicPipelineState = 0;
        void setTextureId(render::TextureId id) { textureId = id; }

        ecs::Entity entity_;

    protected:
        render::TextureId textureId;
        render::MeshId meshId;
        std::uint64_t vertex_shader_hash{0};
        std::uint64_t fragment_shader_hash{0};
        std::int32_t vertex_count{-1};
};
}  // namespace graphics