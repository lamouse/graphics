#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>

#include "model.hpp"

namespace graphics {

struct TransformCompoent {
        ::glm::vec3 translation{};  // position offset
        ::glm::vec3 scale{1.f, 1.f, 1.f};
        ::glm::vec3 rotation{};

        [[nodiscard]] auto mat4() const -> ::glm::mat4 {
            auto transform = ::glm::translate(::glm::mat4(1.f), translation);
            transform = ::glm::rotate(transform, rotation.y, {0.f, 1.f, 0.f});
            transform = ::glm::rotate(transform, rotation.x, {1.f, 0.f, 0.f});
            transform = ::glm::rotate(transform, rotation.z, {0.f, 0.f, 1.f});
            transform = ::glm::scale(transform, scale);
            return transform;
        }
};

class GameObject {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, GameObject>;
        static auto createGameObject() -> GameObject {
            static id_t currentId = 0;
            return {currentId++};
        }

        [[nodiscard]] auto getId() const -> id_t { return id; }
        GameObject(const GameObject&) = delete;
        GameObject(GameObject&&) = default;
        auto operator=(const GameObject&) -> GameObject& = delete;
        auto operator=(GameObject&&) -> GameObject& = default;
        ::std::shared_ptr<Model> model{};
        ::glm::vec3 color{};
        TransformCompoent transform;

    private:
        id_t id;
        GameObject(id_t id) : id(id) {}
};

}  // namespace graphics
