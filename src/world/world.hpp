#pragma once
#include "common/common_funcs.hpp"
#include "world/render_registry.hpp"
#include "resource/id.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/component.hpp"
#include <vector>
#include <functional>
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
        void update(const core::FrameInfo& frameInfo);
        void draw(render::Graphic* gfx);
        template<DrawableLike T>
        void addDrawable(T obj) {
            render_registry_.add(std::forward<T>(obj));
        }
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

        void processOutlineres(const std::function<void(ecs::Outliner&&)>& func) {
            ecs::Outliner outliner;
            outliner.entity = entity_;
            outliner.children = child_entitys_;
            func(std::move(outliner));
            render_registry_.processOutlineres(func);
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
        RenderRegistry render_registry_;
};
}  // namespace world