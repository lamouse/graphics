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

using ModelId = common::SlotId;

namespace graphics {

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
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, ModelInstance>;
        static auto createGameObject(render::TextureId textureId, render::MeshId meshId)
            -> ModelInstance {
            return ModelInstance{currentId++, textureId, meshId};
        }

        // static auto CreateInstance(const ModelResource& resource) -> ModelInstance {

        // }

        [[nodiscard]] auto getModelMatrix() -> glm::mat4 {
            if (entity_.hasComponent<ecs::TransformComponent>()) {
                return entity_.getComponent<ecs::TransformComponent>().mat4();
            }
            return glm::mat4{1.0F};
        }

        [[nodiscard]] auto getId() const -> id_t { return id; }
        ModelInstance(const ModelInstance&) = delete;
        ModelInstance(ModelInstance&&) = default;
        auto operator=(const ModelInstance&) -> ModelInstance& = delete;
        auto operator=(ModelInstance&&) -> ModelInstance& = default;
        [[nodiscard]] auto getEntity() -> ecs::Entity& { return entity_; }
        void drawUI();
        ::std::shared_ptr<Model> model;
        ::glm::vec3 color{};
        ecs::Entity entity_;
        ~ModelInstance() override = default;
        void updateViewProjection(const ecs::Camera& camera);
        [[nodiscard]] auto getUBOData() const -> std::span<const std::byte> override {
            return ubo.as_byte_span();
        };
        [[nodiscard]] auto getTextureId() const -> render::TextureId override { return textureId; }
        [[nodiscard]] auto getMeshId() const -> render::MeshId override { return meshId; }
        void setTextureId(render::TextureId textureId_) override { textureId = textureId_; }
        [[nodiscard]] auto getPrimitiveTopology() const -> render::PrimitiveTopology override {
            return topology;
        }

    private:
        id_t id;
        UniformBufferObject ubo;
        render::TextureId textureId;
        render::MeshId meshId;
        render::PrimitiveTopology topology = render::PrimitiveTopology::Triangles;
        explicit ModelInstance(id_t id, render::TextureId textureId_, render::MeshId meshId_)
            : id(id), textureId(textureId_), meshId(meshId_) {
            static ecs::Scene scene;
            entity_ = scene.createEntity("Model" + std::to_string(id));
            entity_.addComponent<ecs::TransformComponent>();
            entity_.addComponent<ecs::RenderStateComponent>();
        }

        inline static id_t currentId = 0;
};

}  // namespace graphics
