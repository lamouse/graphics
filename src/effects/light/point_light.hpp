#pragma once
#include <glm/glm.hpp>
#include "resource/model_instance.hpp"
#include "common/common_funcs.hpp"
#include "core/frame_info.hpp"
#include "effects/effect.hpp"
#define MAX_LIGHTS 10
namespace graphics::effects {
struct PointLight {
        glm::vec4 position{};  // ignore w
        glm::vec4 color{};     // w is intensity
};

struct PointLightUbo {
        glm::mat4 projection{1.f};
        glm::mat4 view{1.f};
        glm::mat4 inverseView{1.f};
        glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};  // w is intensity
        PointLight pointLights[MAX_LIGHTS];
        int numLights;
        AS_BYTE_SPAN
};

struct PointLightPushConstants {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius;
        AS_BYTE_SPAN
};

class PointLightEffect {
    public:
        PointLightEffect(graphics::ResourceManager& manager, float intensity = 10.f,
                         float radius = 0.5f, glm::vec3 color_ = glm::vec3(1.f))
            : point_light{manager,
                          ModelResourceName{
                              .shader_name = "point_light", .mesh_name = "", .texture_name = ""},
                          "PointLight"},
              color(color_),
              pointLight(std::make_unique<PointLightComponent>()),
              id(getCurrentId()) {
            point_light.entity_.getComponent<ecs::TransformComponent>().scale.x = radius;
            pointLight->lightIntensity = intensity;
            auto rotateLight =
                glm::rotate(glm::mat4(1.f), (1 * glm::two_pi<float>()) / 7, {0.f, -1.f, 0.f});
            point_light.entity_.getComponent<ecs::TransformComponent>().translation =
                glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
            point_light.setVertexCount(6);
            entity_ = getEffectsScene().createEntity("PointLight" + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
        }

        void update(core::FrameInfo& frameInfo) {
            auto rotateLight =
                glm::rotate(glm::mat4(1.f), 0.5f * frameInfo.frameTime, {0.f, -1.f, 0.f});
            auto& transform = point_light.entity_.getComponent<ecs::TransformComponent>();
            // update light position
            transform.translation = glm::vec3(rotateLight * glm::vec4(transform.translation, 1.f));
            point_light.getUBO().numLights = 6;
            point_light.getUBO().projection = frameInfo.camera->getProjection();
            point_light.getUBO().view = frameInfo.camera->getView();
            point_light.getUBO().pointLights[0].position = glm::vec4(transform.translation, 1.f);
            point_light.getUBO().pointLights[0].color =
                glm::vec4(color, pointLight->lightIntensity);
        }
        void draw(render::Graphic* graphic) {
            auto& transform = point_light.entity_.getComponent<ecs::TransformComponent>();
            point_light.PushConstant().position = glm::vec4(transform.translation, 1.f);
            point_light.PushConstant().color = glm::vec4(color, pointLight->lightIntensity);
            point_light.PushConstant().radius = transform.scale.x;
            if (entity_.getComponent<ecs::RenderStateComponent>().visible) {
                graphic->draw(point_light);
            }
        }
        ecs::Entity entity_;

    private:
        struct PointLightComponent {
                float lightIntensity = .3f;
        };
        using PointLightInstance = ModelInstance<PointLightUbo, PointLightPushConstants,
                                                 render::PrimitiveTopology::Triangles>;
        PointLightInstance point_light;
        glm::vec3 color{};
        std::unique_ptr<PointLightComponent> pointLight = nullptr;

        id_t id;
};
}  // namespace graphics::effects