#pragma once

#include <span>
#include <vector>
#include "common/class_traits.hpp"
#include "render_core/vertex.hpp"
#include "render_core/pipeline_state.h"
#include "render_core/types.hpp"
namespace render {
class IMeshData {
    public:
        IMeshData() = default;
        virtual ~IMeshData() = default;
        CLASS_DEFAULT_COPYABLE(IMeshData);
        CLASS_DEFAULT_MOVEABLE(IMeshData);
        [[nodiscard]] virtual auto getMesh() const -> std::span<const float> = 0;
        [[nodiscard]] virtual auto getVertexCount() const -> std::size_t = 0;
        [[nodiscard]] virtual auto getIndices() const -> std::span<const std::byte> = 0;
        [[nodiscard]] virtual auto getIndicesSize() const -> std::uint64_t = 0;
        [[nodiscard]] virtual auto getVertexAttribute() const
            -> std::vector<render::VertexAttribute> = 0;
        [[nodiscard]] virtual auto getVertexBinding() const
            -> std::vector<render::VertexBinding> = 0;
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
              fragment_shader_hash(fragment_shader_hash_) {}
        [[nodiscard]] auto getMeshId() const -> render::MeshId { return meshId; };
        [[nodiscard]] auto vertexShaderHash() const -> std::uint64_t { return vertex_shader_hash; }
        [[nodiscard]] auto fragmentShaderHash() const -> std::uint64_t {
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
};

class ITexture {
    public:
        auto getData() -> unsigned char*;
        [[nodiscard]] virtual auto getWidth() const -> int = 0;
        [[nodiscard]] virtual auto getHeight() const -> int = 0;
        [[nodiscard]] virtual auto getMipLevels() const -> uint32_t = 0;
        [[nodiscard]] virtual auto size() const -> unsigned long long = 0;
        [[nodiscard]] virtual auto data() const -> std::span<unsigned char> = 0;
        [[nodiscard]] auto count() const -> std::uint8_t { return image_count; };
        virtual ~ITexture() = default;

    protected:
        std::uint8_t image_count{1};
};
}  // namespace render