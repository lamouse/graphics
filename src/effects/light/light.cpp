#include "light.hpp"
#include "core/frame_info.hpp"
#include "world/world.hpp"
namespace graphics::effects {
void updateLightUBO(const core::FrameInfo& frameInfo, LightUBO& lightUBO, world::World& world) {
    auto lights = world.getLightEntities();

    lightUBO.projection = frameInfo.camera->getProjection();
    lightUBO.view = frameInfo.camera->getView();
    lightUBO.inverseView = frameInfo.camera->getView();
    lightUBO.ambientLightColor = glm::vec4{1.f, 1.f, 1.f, .04f};
    int index = 0;
    for (const auto& light : lights) {
        if (light.light->type == ecs::LightType::Point) {
            PointLight point_light{};

            point_light.color = {light.light->color, light.light->intensity};
            point_light.position = {light.transform->translation, 1.0f};
            lightUBO.pointLights[index] = point_light;
            index++;
            if (index >= MAX_LIGHTS) {
                break;
            }
        } else if (light.light->type == ecs::LightType::Directional) {
            DirLight dirLight{};
            dirLight.direction = glm::vec4(glm::normalize(light.light->direction), 0.f);
            dirLight.color = glm::vec4(light.light->color, light.light->intensity);
            lightUBO.dirLight = dirLight;

            // TODO 临时测试
            lightUBO.spotLight.position =
                glm::vec4(frameInfo.camera->getPosition(), light.light->outerCone);
            lightUBO.spotLight.direction =
                glm::vec4(frameInfo.camera->front(), light.light->innerCone);
            lightUBO.spotLight.color = glm::vec4(light.light->color, 1);
            // TODO 临时测试
        }
    }
    lightUBO.numLights = index;
}
}  // namespace graphics::effects