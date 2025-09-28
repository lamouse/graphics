#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>

#include "resource/obj/model.hpp"

namespace graphics {

struct TransformComponent {
        ::glm::vec3 translation{};  // position offset
        ::glm::vec3 scale{1.F};
        ::glm::vec3 rotation{};

        [[nodiscard]] auto mat4() const -> ::glm::mat4 {
            auto transform = ::glm::translate(::glm::mat4(1.F), translation);
            transform = ::glm::rotate(transform, rotation.y, {0.F, 1.F, 0.F});
            transform = ::glm::rotate(transform, rotation.x, {1.F, 0.F, 0.F});
            transform = ::glm::rotate(transform, rotation.z, {0.F, 0.F, 1.F});
            transform = ::glm::scale(transform, scale);
            return transform;
        }
        [[nodiscard]] auto rotate(float angle) const -> ::glm::mat4 {
            return ::glm::rotate(glm::mat4(1.0F), angle,
                                rotation);
        }
};

class ModelInstance {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, ModelInstance>;
        static auto createGameObject() -> ModelInstance {
            static id_t currentId = 0;
            return ModelInstance{currentId++};
        }

        [[nodiscard]] auto getId() const -> id_t { return id; }
        ModelInstance(const ModelInstance&) = delete;
        ModelInstance(ModelInstance&&) = default;
        auto operator=(const ModelInstance&) -> ModelInstance& = delete;
        auto operator=(ModelInstance&&) -> ModelInstance& = default;
        ::std::shared_ptr<Model> model;
        ::glm::vec3 color{};
        TransformComponent transform;
        ~ModelInstance() = default;
    private:
        id_t id;
        explicit ModelInstance(id_t id) : id(id) {}
};

}  // namespace graphics
