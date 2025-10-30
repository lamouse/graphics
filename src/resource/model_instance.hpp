#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include "ecs/scene/entity.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/components/render_state_component.hpp"
#include "ecs/components/camera_component.hpp"
#include "resource/obj/model.hpp"
#include "resource/instance.hpp"
#include "resource/id.hpp"
#include "resource/resource.hpp"
#include <type_traits>

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

struct ModelResourceName {
        std::string shader_name;
        std::string mesh_name;
        std::string texture_name;
};

template <typename UBO, render::PrimitiveTopology primitiveTopology>
    requires ByteSpanConvertible<UBO> && std::is_trivially_copyable_v<UBO>
class ModelInstance : public IModelInstance {
    public:
        [[nodiscard]] auto getId() const -> id_t { return id; }
        ModelInstance(const ModelInstance&) = delete;
        ModelInstance(ModelInstance&&) = default;
        auto operator=(const ModelInstance&) -> ModelInstance& = delete;
        auto operator=(ModelInstance&&) -> ModelInstance& = default;
        [[nodiscard]] auto getEntity() -> ecs::Entity& { return entity_; }
        ::std::shared_ptr<Model> model;
        ::glm::vec3 color{};
        ~ModelInstance() override = default;
        [[nodiscard]] auto getUBOData() const -> std::span<const std::byte> override {
            return ubo.as_byte_span();
        };
        [[nodiscard]] auto getPrimitiveTopology() const -> render::PrimitiveTopology override {
            return topology;
        }
        void updateViewProjection(const ecs::Camera& camera) {
            auto& transform = entity_.getComponent<ecs::TransformComponent>();
            ubo.model = transform.mat4();
            ubo.view = camera.getView();
            ubo.proj = camera.getProjection();
        }
        auto getUBO() -> UBO& { return ubo; }

        ModelInstance(const ResourceManager& resource, const ModelResourceName& resourceName,
                      const std::string& modelName)
            : id(getCurrentId()),
              texture_name(resourceName.texture_name),
              mesh_name(resourceName.mesh_name),
              shader_name(resourceName.shader_name) {
            textureId = resource.getTexture(texture_name);
            meshId = resource.getMesh(mesh_name);
            auto hash = resource.getShaderHash<ShaderHash>(shader_name);
            vertex_shader_hash = hash.vertex;
            fragment_shader_hash = hash.fragment;
            entity_ = getModelScene().createEntity(
                modelName.empty() ? "Model" + std::to_string(id) : modelName + std::to_string(id));
            entity_.addComponent<ecs::TransformComponent>();
            entity_.addComponent<ecs::RenderStateComponent>(id);
        }

    private:
        id_t id;
        UBO ubo{};
        std::string texture_name;
        std::string mesh_name;
        std::string shader_name;
        render::PrimitiveTopology topology = primitiveTopology;
};

using Base3DModelInstance = ModelInstance<UniformBufferObject, render::PrimitiveTopology::Triangles>;

}  // namespace graphics
