#pragma once
#include <glm/glm.hpp>
#include "resource/mesh_instance.hpp"
#include "common/common_funcs.hpp"
#include "core/frame_info.hpp"
#include "effects/effect.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/components/light_component.hpp"
#include "world/world.hpp"
#define MAX_LIGHTS 10
namespace graphics::effects {
struct PointLight {
        glm::vec4 position{};  // ignore w
        glm::vec4 color{};     // w is intensity
        CLASS_DEFAULT_COPYABLE(PointLight);
        CLASS_DEFAULT_MOVEABLE(PointLight);
        PointLight() = default;
        ~PointLight() = default;
};

struct LightUBO {
        glm::mat4 projection{1.f};
        glm::mat4 view{1.f};
        glm::mat4 inverseView{1.f};
        glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};  // w is intensity
        PointLight pointLights[MAX_LIGHTS];
        int numLights{};
        AS_BYTE_SPAN
};

struct PointLightPushConstants {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius{};
        AS_BYTE_SPAN
};

class PointLightEffect {
    public:
        PointLightEffect(graphics::ResourceManager& manager,
                         const layout::FrameBufferLayout& layout, float intensity = 10.f,
                         float radius = .02f, glm::vec3 color_ = glm::vec3(1.f, 1.f, 1.f))
            : id(getCurrentId()) {
            auto hash = manager.getShaderHash<ShaderHash>("point_light");
            point_light = PointLightInstance{{}, hash, layout, "PointLightInstance"};
            auto rotateLight =
                glm::rotate(glm::mat4(1.f), (static_cast<float>(id) * glm::two_pi<float>()) / 7,
                            {0.f, -1.f, 0.f});
            point_light.setVertexCount(6);
            entity_ = getEffectsScene().createEntity("PointLight: " + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
            ecs::TransformComponent transformComponent{};
            transformComponent.translation = glm::vec3(rotateLight * glm::vec4(-2.f, .0f, -1.f, 1.f));
            entity_.addComponent<ecs::TransformComponent>(transformComponent);

            ecs::LightComponent lightComponent{};
            lightComponent.color = color_;
            lightComponent.intensity = intensity;
            lightComponent.range = radius;
            entity_.addComponent<ecs::LightComponent>(lightComponent);
        }

        CLASS_NON_COPYABLE(PointLightEffect);
        CLASS_DEFAULT_MOVEABLE(PointLightEffect);

        void update(const core::FrameInfo& frameInfo, world::World& world) {
            auto& transform = entity_.getComponent<ecs::TransformComponent>();
            constexpr float angularSpeed = .5f;  // 弧度/秒
            float angle = angularSpeed * frameInfo.frameTime;
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec4 localOffset(transform.translation, 1.f);

            transform.translation =  glm::vec3(rotation * localOffset);
            point_light.getUBO<LightUBO>().projection = frameInfo.camera->getProjection();
            point_light.getUBO<LightUBO>().view = frameInfo.camera->getView();

            world.addLightEntity(entity_);
        }
        void draw(render::Graphic* graphic) {
            auto& transform = entity_.getComponent<ecs::TransformComponent>();
            auto lightComponent = entity_.getComponent<ecs::LightComponent>();
            point_light.PushConstant().position = glm::vec4(transform.translation, 1.f);
            point_light.PushConstant().color =
                glm::vec4(lightComponent.color, lightComponent.intensity);
            point_light.PushConstant().radius = lightComponent.range;
            if (entity_.getComponent<ecs::RenderStateComponent>().visible) {
                graphic->draw(point_light);
            }
        }
        ecs::Entity entity_;
        [[nodiscard]] auto getChildEntitys() const -> std::vector<ecs::Entity> {
            return std::vector{point_light.entity_};
        }

    private:
        using PointLightInstance =
            MeshInstance<PointLightPushConstants, render::PrimitiveTopology::Triangles,
                         LightUBO>;
        PointLightInstance point_light;
        id_t id;
};
}  // namespace graphics::effects