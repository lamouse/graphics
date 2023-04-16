#pragma once
#include "g_model.hpp"
#include <memory>
#include <glm/gtc/matrix_transform.hpp>
#include <stdint.h>
#include "resource/image.hpp"
namespace g
{

using id_t = unsigned int;

struct TransformCompoent
{
    ::glm::vec3 translation{}; //position offset
    ::glm::vec3 scale{1.f, 1.f, 1.f};
    ::glm::vec3 rotation{};

    ::glm::mat4 mat4(){
        auto transform = ::glm::translate(::glm::mat4(1.f), translation);
        transform = ::glm::rotate(transform, rotation.y, {0.f, 1.f, 0.f});
        transform = ::glm::rotate(transform, rotation.x, {1.f, 0.f, 0.f});
        transform = ::glm::rotate(transform, rotation.z, {0.f, 0.f, 1.f});
        transform = ::glm::scale(transform, scale);
        return transform;
    }
};


class GameObject
{
private:
    id_t id;
    GameObject(id_t id):id(id){}

public:
    static GameObject createGameObject(){
        static id_t currentId = 0;
        return GameObject(currentId++);
    }

    id_t getId(){return id;}
    GameObject(const GameObject&) = delete;
    GameObject(GameObject&&) = default;
    GameObject operator=(const GameObject&) = delete;
    GameObject& operator=(GameObject&&) = default;
    ::std::shared_ptr<Model> model{};
    ::glm::vec3 color{};
    TransformCompoent transform;
    ~GameObject();
};

    
} 
