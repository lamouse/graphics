#pragma once
#include <glm/glm.hpp>
#include "render_core/mesh.hpp"
namespace graphics {
struct ParticleModel : public render::IMeshData {
        struct Vertex {
                glm::vec2 position;
                glm::vec2 velocity;
                glm::vec4 color;

                static auto getVertexBinding() -> std::vector<render::VertexBinding> {
                    std::vector<render::VertexBinding> bindings;
                    bindings.push_back(render::VertexBinding{.binding = 0,
                                                             .stride = sizeof(Vertex),
                                                             .is_instance = false,
                                                             .divisor = 1});
                    return bindings;
                }
                static auto getVertexAttribute() -> std::vector<render::VertexAttribute> {
                    std::vector<render::VertexAttribute> vertex_attributes;

                    render::VertexAttribute position{};
                    position.hex = 0;
                    position.location.Assign(0);
                    position.type.Assign(render::VertexAttribute::Type::Float);
                    position.offset.Assign(offsetof(Vertex, position));
                    position.size.Assign(render::VertexAttribute::Size::R32_G32);
                    vertex_attributes.push_back(position);

                    render::VertexAttribute color{};
                    color.hex = 0;
                    color.location.Assign(1);
                    color.type.Assign(render::VertexAttribute::Type::Float);
                    color.offset.Assign(offsetof(Vertex, color));
                    color.size.Assign(render::VertexAttribute::Size::R32_G32_B32_A32);
                    vertex_attributes.push_back(color);
                    return vertex_attributes;
                }
        };
        [[nodiscard]] auto getMesh() const -> std::span<const float> override;
        [[nodiscard]] auto getVertexCount() const -> std::size_t override {
            return particles.size();
        }
        [[nodiscard]] auto getIndices() const -> std::span<const std::byte> override { return {}; }
        [[nodiscard]] auto getIndicesSize() const -> std::uint64_t override { return {}; }

        [[nodiscard]] auto getVertexAttribute() const
            -> std::vector<render::VertexAttribute> override {
            return Vertex::getVertexAttribute();
        }
        [[nodiscard]] auto getVertexBinding() const -> std::vector<render::VertexBinding> override {
            return Vertex::getVertexBinding();
        }

        ParticleModel(std::uint64_t count, float aspect);
        ~ParticleModel() override = default;

    private:
        // Initial particle positions on a circle
        std::vector<Vertex> particles;
};
}  // namespace graphics