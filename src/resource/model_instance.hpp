#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>
#include "ecs/scene/entity.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/components/render_state_component.hpp"
#include "ecs/components/camera_component.hpp"
#include "resource/obj/model.hpp"
#include "common/slot_vector.hpp"
#include "resource/instance.hpp"


namespace graphics {
class ResourceManager;

struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        // 转换为 std::span<std::byte>
        auto as_byte_span() -> std::span<const std::byte> {
            return std::span<const std::byte>{reinterpret_cast<const std::byte*>(this),
                                              sizeof(UniformBufferObject)};
        }

        [[nodiscard]] auto as_byte_span() const -> std::span<const std::byte> {
            return std::span<const std::byte>{reinterpret_cast<const std::byte*>(this),
                                              sizeof(UniformBufferObject)};
        }
};

struct ModelResource {
        std::string vertex_shader_name;
        std::string fragment_shader_name;
        std::string texture_name;
};

class ModelInstance : public IModelInstance {
    public:
        [[nodiscard]] auto getId() const -> id_t { return id; }
        ModelInstance(const ModelInstance&) = delete;
        ModelInstance(ModelInstance&&) = default;
        auto operator=(const ModelInstance&) -> ModelInstance& = delete;
        auto operator=(ModelInstance&&) -> ModelInstance& = default;
        [[nodiscard]] auto getEntity() -> ecs::Entity& { return entity_; }
        void drawUI();
        ::std::shared_ptr<Model> model;
        ::glm::vec3 color{};
        ~ModelInstance() override = default;
        void updateViewProjection(const ecs::Camera& camera);
        [[nodiscard]] auto getUBOData() const -> std::span<const std::byte> override {
            return ubo.as_byte_span();
        };
        [[nodiscard]] auto getPrimitiveTopology() const -> render::PrimitiveTopology override {
            return topology;
        }
        explicit ModelInstance(const ResourceManager& resource, const std::string& textureName,
                               const std::string& meshName, std::string shaderName);

    private:
        id_t id;
        UniformBufferObject ubo{};
        std::string texture;
        std::string mesh;
        std::string shader;
        render::PrimitiveTopology topology = render::PrimitiveTopology::Triangles;
        explicit ModelInstance(id_t id, render::TextureId textureId_, render::MeshId meshId_)
            : id(id) {
            textureId = textureId_;
            meshId = meshId_;
            entity_ = scene.createEntity("Model" + std::to_string(id));
            entity_.addComponent<ecs::TransformComponent>();
            entity_.addComponent<ecs::RenderStateComponent>(id);
        }
};

}  // namespace graphics
