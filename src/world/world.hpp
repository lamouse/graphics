#pragma once
#include "common/common_funcs.hpp"
#include "world/render_registry.hpp"
#include "resource/id.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/component.hpp"
#include <vector>
#include <functional>

namespace core {
class FrameTime;
namespace frontend {
class BaseWindow;
}
}  // namespace core

namespace graphics {
class ResourceManager;
namespace input {
class InputSystem;
class Mouse;
}
}  // namespace graphics

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
        void update(core::frontend::BaseWindow& window, graphics::ResourceManager& resourceManager,
                    graphics::input::InputSystem& input_system);
        void draw(render::Graphic* gfx);
        template <DrawableLike T>
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
            auto* draw_able = render_registry_.getDrawableById(id);
            if (draw_able) {
                auto& render_state =
                    draw_able->getEntity().getComponent<ecs::RenderStateComponent>();
                render_state.mouse_select = true;
                render_state.select_id = id;
            }
        }

        void cancelPick() {
            auto* draw_able = render_registry_.getDrawableById(pick_id);
            if (draw_able) {
                auto& render_state =
                    draw_able->getEntity().getComponent<ecs::RenderStateComponent>();
                render_state.mouse_select = false;
                render_state.select_id = std::numeric_limits<unsigned int>::max();
            }
        };

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

        void process_mouse_input(core::FrameInfo& frameInfo, graphics::input::Mouse* mouse);
        id_t id_;
        id_t pick_id{0};
        bool is_pick{false};
        ecs::CameraComponent* cameraComponent_{nullptr};
        ecs::Scene scene_;
        ecs::Entity cameraEntity_;
        ecs::Entity dirLightEntity_;
        ecs::LightComponent* dir_light;
        std::vector<LightInfo> lights_;
        std::vector<ecs::Entity> child_entitys_;
        RenderRegistry render_registry_;
        std::unique_ptr<core::FrameTime> frame_time_;
};
}  // namespace world