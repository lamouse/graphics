#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>
#include "ecs/scene/entity.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/components/transform_component.hpp"
#include "resource/obj/model.hpp"
#include "common/slot_vector.hpp"
#include "render_core/types.hpp"
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
};

class IModelInstance {
    public:
        IModelInstance() = default;
        virtual ~IModelInstance() = default;
        CLASS_DEFAULT_COPYABLE(IModelInstance);
        CLASS_DEFAULT_MOVEABLE(IModelInstance);
        [[nodiscard]] virtual auto getTextureId() const -> render::TextureId = 0;
        virtual void setTextureId(render::TextureId) = 0;
        [[nodiscard]] virtual auto getMeshId() const -> render::MeshId = 0;
        [[nodiscard]] virtual auto getUBOData() const -> std::span<const std::byte> = 0;
};

class ModelInstance : public IModelInstance {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, ModelInstance>;
        static auto createGameObject(render::TextureId textureId, render::MeshId meshId)
            -> ModelInstance {
            static id_t currentId = 0;
            return ModelInstance{currentId++, textureId, meshId};
        }
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
        void writeToUBOMapData(std::span<const std::byte> data);
        [[nodiscard]] auto getUBOData() const -> std::span<const std::byte> override {
            // NOLINTNEXTLINE
            ASSERT_MSG(!ubo_data.empty(), "UBO size not match");
            return ubo_data;
        };
        [[nodiscard]] auto getTextureId() const -> render::TextureId override { return textureId; }
        [[nodiscard]] auto getMeshId() const -> render::MeshId override { return meshId; }
        void setTextureId(render::TextureId textureId_) override { textureId = textureId_; }

    private:
        id_t id;
        std::span<const std::byte> ubo_data;
        render::TextureId textureId;
        render::MeshId meshId;
        explicit ModelInstance(id_t id, render::TextureId textureId_, render::MeshId meshId_)
            : id(id), textureId(textureId_), meshId(meshId_) {
            static ecs::Scene scene;
            entity_ = scene.createEntity("ModelInstance");
            entity_.addComponent<ecs::TransformComponent>();
        }
};

}  // namespace graphics
