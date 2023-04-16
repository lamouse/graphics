#include <cstdio>
#include <unordered_map>
class GameObject
{

public:
    using id_t = unsigned int;
    using Map = ::std::unordered_map<id_t, GameObject>;
    static GameObject createGameObject(){
        static id_t currentId = 0;
        return GameObject(currentId++);
    }

    id_t getId(){return id;}
    GameObject(const GameObject&) = delete;
    GameObject(GameObject&&) = default;
    GameObject& operator=(const GameObject&) = delete;
    GameObject& operator=(GameObject&&) = default;

    private:
        id_t id;
        GameObject(id_t id):id(id){}
};
GameObject::Map gameObjects;

int main()
{
    auto cube = GameObject::createGameObject();
    gameObjects.emplace(cube.getId(), ::std::move(cube));
}