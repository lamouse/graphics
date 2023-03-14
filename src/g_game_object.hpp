#pragma once
#include "g_model.hpp"
#include<memory>
namespace g
{

using id_t = unsigned int;

struct Transform2dCompoent
{
    ::glm::vec2 translation{}; //position offset
    ::glm::vec2 scale{1.f, 1.f};
    float rotation;

    ::glm::mat2 mat2(){
        const float s = ::glm::sin(rotation);
        const float c = ::glm::cos(rotation);
        ::glm:: mat2 rotMatrix{{c, s}, {-s, c}};
        ::glm::mat2 scaleMat{{scale.x, 0.f}, {.0f, scale.y}};
        return rotMatrix * ::glm::mat2{1.f};
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
    Transform2dCompoent transform2d;
    ~GameObject();
};

    
} 
