#pragma once
#include <glm/glm.hpp>
#include "resource/mesh_instance.hpp"
#include "common/common_funcs.hpp"
#include "core/frame_info.hpp"
#include "effects/effect.hpp"
#include "effects/light/light.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/components/light_component.hpp"
#include "world/world.hpp"
#include "render_core/graphic.hpp"

namespace graphics::effects {

struct PointLightPushConstants {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius{};
        AS_BYTE_SPAN
};

struct PiintLightCreateInfo {
        ShaderHash hash;
        glm::vec3 position;
        float intensity;
        float radius;
        glm::vec3 color;
};

class PointLightEffect {
    public:
        PointLightEffect(PiintLightCreateInfo& createInfo)
            : point_light(PointLightInstance{{}, createInfo.hash, "PointLightInstance"}),
              id(getCurrentId()) {
            auto rotateLight =
                glm::rotate(glm::mat4(1.f), (static_cast<float>(id) * glm::two_pi<float>()) / 7,
                            {0.f, -1.f, 0.f});
            point_light.setVertexCount(6);
            entity_ = getEffectsScene().createEntity("PointLight: " + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
            render_state = &entity_.getComponent<ecs::RenderStateComponent>();  // NOLINT
            ecs::TransformComponent transformComponent{};
            transformComponent.translation =
                glm::vec3(rotateLight * glm::vec4(-2.f, .0f, -1.f, 1.f));
            entity_.addComponent<ecs::TransformComponent>(transformComponent);
            transform = &entity_.getComponent<ecs::TransformComponent>();  // NOLINT
            ecs::LightComponent default_lightComponent{};
            default_lightComponent.color = createInfo.color;
            default_lightComponent.intensity = createInfo.intensity;
            default_lightComponent.range = createInfo.radius;
            entity_.addComponent<ecs::LightComponent>(default_lightComponent);
            lightComponent = &entity_.getComponent<ecs::LightComponent>();  // NOLINT
            point_light.setUBO<LightUBO>(&light_ubo);
            point_light.setPushConstant(&push_constants);
        }
        ~PointLightEffect() = default;
        PointLightEffect(graphics::ResourceManager& manager, float intensity = 10.f,
                         float radius = .02f, glm::vec3 color_ = glm::vec3(1.f, 1.f, 1.f))
            : id(getCurrentId()) {
            auto hash = manager.getShaderHash<ShaderHash>("point_light");
            point_light = PointLightInstance{{}, hash, "PointLightInstance"};
            auto rotateLight =
                glm::rotate(glm::mat4(1.f), (static_cast<float>(id) * glm::two_pi<float>()) / 7,
                            {0.f, -1.f, 0.f});
            point_light.setVertexCount(6);
            entity_ = getEffectsScene().createEntity("PointLight: " + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
            render_state = &entity_.getComponent<ecs::RenderStateComponent>();  // NOLINT
            ecs::TransformComponent transformComponent{};
            transformComponent.translation =
                glm::vec3(rotateLight * glm::vec4(-2.f, .0f, -1.f, 1.f));
            entity_.addComponent<ecs::TransformComponent>(transformComponent);
            transform = &entity_.getComponent<ecs::TransformComponent>();  // NOLINT
            ecs::LightComponent default_lightComponent{};
            default_lightComponent.color = color_;
            default_lightComponent.intensity = intensity;
            default_lightComponent.range = radius;
            entity_.addComponent<ecs::LightComponent>(default_lightComponent);
            lightComponent = &entity_.getComponent<ecs::LightComponent>();  // NOLINT
            point_light.setUBO<LightUBO>(&light_ubo);
            point_light.setPushConstant(&push_constants);
        }

        CLASS_NON_COPYABLE(PointLightEffect);
        CLASS_DEFAULT_MOVEABLE(PointLightEffect);

        void update(const core::FrameInfo& frameInfo, world::World& world) {
            constexpr float angularSpeed = .5f;  // 弧度/秒
            float angle = angularSpeed * static_cast<float>(frameInfo.frame_time.frame);
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec4 localOffset(transform->translation, 1.f);

            transform->translation = glm::vec3(rotation * localOffset);
            light_ubo.projection = frameInfo.camera->getProjection();
            light_ubo.view = frameInfo.camera->getView();

            world.addLight({.id = id, .light = lightComponent, .transform = transform});
        }
        void draw(render::Graphic* graphic) {
            push_constants.position = glm::vec4(transform->translation, 1.f);
            push_constants.color = glm::vec4(lightComponent->color, lightComponent->intensity);
            push_constants.radius = lightComponent->range;
            if (render_state->visible) {
                graphic->draw(point_light);
            }
        }
        ecs::Entity entity_;
        [[nodiscard]] auto getChildEntitys() const -> std::vector<ecs::Entity> {
            return std::vector{point_light.entity_};
        }

        auto getId() const -> id_t { return id; }

    private:
        using PointLightInstance =
            MeshInstance<PointLightPushConstants, render::PrimitiveTopology::Triangles, LightUBO>;
        LightUBO light_ubo;
        PointLightPushConstants push_constants;
        PointLightInstance point_light;
        ecs::RenderStateComponent* render_state{};
        ecs::TransformComponent* transform{};
        ecs::LightComponent* lightComponent{};
        id_t id;
};
}  // namespace graphics::effects