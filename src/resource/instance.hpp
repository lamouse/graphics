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

class IMeshInstance {
    public:
        IMeshInstance() = default;
        virtual ~IMeshInstance() = default;
        CLASS_DEFAULT_COPYABLE(IMeshInstance);
        CLASS_DEFAULT_MOVEABLE(IMeshInstance);
        IMeshInstance(render::PrimitiveTopology topology_, render::RenderCommand render_command_,
                      render::MeshId meshId_, std::uint64_t vertex_shader_hash_,
                      std::uint64_t fragment_shader_hash_)
            : topology(topology_),
              render_command(render_command_),
              meshId(meshId_),
              vertex_shader_hash(vertex_shader_hash_),
              fragment_shader_hash(fragment_shader_hash_),
              id(getCurrentId()) {}
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
        [[nodiscard]] virtual auto getPushConstants() const -> std::span<const std::byte> = 0;
        [[nodiscard]] virtual auto getMaterialIds() const -> std::span<const render::TextureId> = 0;
        [[nodiscard]] auto getPrimitiveTopology() const -> render::PrimitiveTopology {
            return topology;
        }
        [[nodiscard]] virtual auto getPipelineState() const -> render::DynamicPipelineState = 0;
        [[nodiscard]] virtual auto getUBOs() const -> std::span<std::span<const std::byte>> = 0;
        [[nodiscard]] auto getRenderCommand() const -> render::RenderCommand {
            return render_command;
        }
        [[nodiscard]] auto getId() const -> id_t { return id; }
        template <typename T>
        void setUBO(uint32_t binding, const T& data) {
            static_assert(std::is_trivially_copyable_v<T>, "UBO must be trivially copyable");
            std::vector<std::byte> buf(sizeof(T));
            std::memcpy(buf.data(), &data, sizeof(T));

            if (binding >= ubo_buffers_.size()) {
                ubo_buffers_.resize(binding + 1);
            }
            ubo_buffers_[binding] = std::move(buf);
        }

    protected:
        render::PrimitiveTopology topology;
        render::RenderCommand render_command;
        render::MeshId meshId;
        std::uint64_t vertex_shader_hash{0};
        std::uint64_t fragment_shader_hash{0};
        std::int32_t vertex_count{-1};
        std::vector<std::vector<std::byte>> ubo_buffers_;
        MeshMaterialResource material_resource_;
        id_t id{};
};
}  // namespace graphics