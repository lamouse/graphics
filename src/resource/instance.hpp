#pragma once
#include "common/common_funcs.hpp"
#include "common/assert.hpp"
#include "render_core/types.hpp"
#include <span>
#include "render_core/fixed_pipeline_state.h"
#include "ecs/scene/entity.hpp"
namespace graphics {

auto getModelScene() -> ecs::Scene&;

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
        [[nodiscard]] virtual auto getUBOData() const -> std::span<const std::byte> = 0;
        [[nodiscard]] virtual auto getPrimitiveTopology() const -> render::PrimitiveTopology = 0;
        void setTextureId(render::TextureId id) { textureId = id; }

        ecs::Entity entity_;

    protected:
        render::TextureId textureId;
        render::MeshId meshId;
        std::uint64_t vertex_shader_hash{0};
        std::uint64_t fragment_shader_hash{0};
};
}  // namespace graphics