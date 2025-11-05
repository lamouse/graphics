#pragma once
#include "common/common_funcs.hpp"
#include "common/assert.hpp"
#include "render_core/types.hpp"
#include <span>
#include "render_core/pipeline_state.h"
#include "ecs/scene/entity.hpp"
#include <spdlog/spdlog.h>
#include "resource/id.hpp"
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

class IMeshInstance {
    public:
        IMeshInstance() = default;
        virtual ~IMeshInstance() = default;
        CLASS_DEFAULT_COPYABLE(IMeshInstance);
        CLASS_DEFAULT_MOVEABLE(IMeshInstance);
        IMeshInstance(render::RenderCommand render_command_, render::MeshId meshId_, render::TextureId textureId_,
                      std::uint64_t vertex_shader_hash_, std::uint64_t fragment_shader_hash_)
            : render_command(render_command_),
            textureId(textureId_),
              meshId(meshId_),
              vertex_shader_hash(vertex_shader_hash_),
              fragment_shader_hash(fragment_shader_hash_),
              id(getCurrentId()) {}
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

        void setTextureId(render::TextureId id_) { textureId = id_; }
        [[nodiscard]] auto getRenderCommand() const -> render::RenderCommand {
            return render_command;
        }
        ecs::Entity entity_;
        [[nodiscard]] auto getId() const -> id_t { return id; }

    protected:
        render::RenderCommand render_command;
        render::TextureId textureId;
        render::MeshId meshId;
        std::uint64_t vertex_shader_hash{0};
        std::uint64_t fragment_shader_hash{0};
        std::int32_t vertex_count{-1};
        id_t id{};
};
}  // namespace graphics