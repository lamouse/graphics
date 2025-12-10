#pragma once
#include "common/common_funcs.hpp"
#include "resource/id.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/scene/entity.hpp"
#include "ecs/component.hpp"
#include <vector>
namespace world {

enum class WorldEntityType : std::uint8_t { CAMERA };

class World {
        struct LightInfo {
                ecs::LightComponent* light;
                ecs::TransformComponent* transform;
        };

    public:
        World();
        [[nodiscard]] auto getEntity(WorldEntityType entityType) const -> ecs::Entity;
        void addLight(const LightInfo& info) { lights_.push_back(info); }
        [[nodiscard]] auto getLightEntities(this auto&& self) -> decltype(auto) {
            {
                return (std::span(self.lights_));
            }
        }

        void pick(id_t id) {
            pick_id = id;
            is_pick = true;
        }

        void cancelPick() { is_pick = false; };
        // 如果选中有效返回选中的id和true，否则返回任意id和false
        [[nodiscard]] auto pick() const -> std::pair<id_t, bool> {
            return std::make_pair(pick_id, is_pick);
        }

        [[nodiscard]] auto getScene() -> ecs::Scene&;
        ~World();
        CLASS_DEFAULT_MOVEABLE(World);
        CLASS_NON_COPYABLE(World);

        void cleanLight() {
            lights_.clear();
            lights_.push_back({.light = dir_light, .transform = nullptr});
        }

        [[nodiscard]] auto getEntities() const -> std::vector<ecs::Entity> {
            return child_entitys_;
        }

        ecs::Entity entity_;

    private:
        id_t id_;
        id_t pick_id;
        bool is_pick{false};
        ecs::Scene scene_;
        ecs::Entity cameraEntity_;
        ecs::Entity dirLightEntity_;
        ecs::LightComponent* dir_light;
        std::vector<LightInfo> lights_;
        std::vector<ecs::Entity> child_entitys_;
};
}  // namespace world