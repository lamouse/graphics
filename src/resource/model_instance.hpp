#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>
#include <utility>
#include "ecs/scene/entity.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/components/transform_component.hpp"
#include "resource/obj/model.hpp"
#include "resource/texture/image.hpp"
#include "common/slot_vector.hpp"
using ModelId = common::SlotId;

namespace graphics {

class ModelInstance {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, ModelInstance>;
        static auto createGameObject(std::string image_path, std::string mode_path)
            -> ModelInstance {
            static id_t currentId = 0;
            return ModelInstance{currentId++, std::move(image_path), std::move(mode_path)};
        }
        [[nodiscard]] auto getModelMatrix() -> glm::mat4 {
            if (entity_.hasComponent<ecs::TransformComponent>()) {
                return entity_.getComponent<ecs::TransformComponent>().mat4();
            }
            return glm::mat4{1.0F};
        }

        [[nodiscard]] auto getId() const -> id_t { return id; }
        [[nodiscard]] auto getImageData() const -> std::unique_ptr<resource::image::Image>;
        ModelInstance(const ModelInstance&) = delete;
        ModelInstance(ModelInstance&&) = default;
        auto operator=(const ModelInstance&) -> ModelInstance& = delete;
        auto operator=(ModelInstance&&) -> ModelInstance& = default;
        [[nodiscard]] auto getEntity() -> ecs::Entity& { return entity_; }
        void drawUI();
        ::std::shared_ptr<Model> model;
        ::glm::vec3 color{};
        ecs::Entity entity_;
        ~ModelInstance() = default;

    private:
        id_t id;
        ModelId modelId;
        std::string image_path;
        std::string mode_path;
        explicit ModelInstance(id_t id, std::string image_path, std::string mode_path)
            : id(id), image_path(std::move(image_path)), mode_path(std::move(mode_path)) {
            static ecs::Scene scene;
            entity_ = scene.createEntity("ModelInstance");
            entity_.addComponent<ecs::TransformComponent>();
        }
};

}  // namespace graphics
