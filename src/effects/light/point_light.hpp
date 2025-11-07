#pragma once
#include <glm/glm.hpp>
#include "resource/mesh_instance.hpp"
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
        PointLightEffect(graphics::ResourceManager& manager,
                         const layout::FrameBufferLayout& layout, float intensity = 10.f,
                         float radius = .4f, glm::vec3 color_ = glm::vec3(.2f, .3f, .4f))
            : color(color_), pointLight(), id(getCurrentId()) {
            auto hash = manager.getShaderHash<ShaderHash>("point_light");
            point_light = PointLightInstance{{}, hash, layout, "PointLightInstance"},
            point_light.entity_.getComponent<ecs::TransformComponent>().scale.x = radius;
            pointLight.lightIntensity = intensity;
            auto rotateLight =
                glm::rotate(glm::mat4(1.f), (1 * glm::two_pi<float>()) / 7, {0.f, -1.f, 0.f});
            point_light.entity_.getComponent<ecs::TransformComponent>().translation =
                glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
            point_light.setVertexCount(6);
            entity_ = getEffectsScene().createEntity("PointLight" + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
        }

        CLASS_NON_COPYABLE(PointLightEffect);
        CLASS_DEFAULT_MOVEABLE(PointLightEffect);

        void update(const core::FrameInfo& frameInfo) {
            // 2. 定义旋转参数
            constexpr float angularSpeed = 1.4f;  // 弧度/秒
            constexpr float radius = 2.0f;        // 旋转半径
                                                  // 3. 计算当前角度（绝对角度，不是增量！）
            float angle = angularSpeed * frameInfo.frameTime;
            auto& transform = point_light.entity_.getComponent<ecs::TransformComponent>();
            // 4. 构造旋转矩阵（绕 Y 轴）
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
            // 5. 初始偏移向量（在 XZ 平面）
            glm::vec4 localOffset(radius, 0.0f, 0.0f, 1.0f);

            // 6. 计算世界位置：center + 旋转后的偏移
            glm::vec3 newPosition = glm::vec3(rotation * localOffset);

            transform.translation = newPosition;
            point_light.getUBO<PointLightUbo>().numLights = 6;
            point_light.getUBO<PointLightUbo>().projection = frameInfo.camera->getProjection();
            point_light.getUBO<PointLightUbo>().view = frameInfo.camera->getView();
            point_light.getUBO<PointLightUbo>().pointLights[0].position = glm::vec4(transform.translation, 1.f);
            point_light.getUBO<PointLightUbo>().pointLights[0].color = glm::vec4(color, pointLight.lightIntensity);
        }
        void draw(render::Graphic* graphic) {
            auto& transform = point_light.entity_.getComponent<ecs::TransformComponent>();
            point_light.PushConstant().position = glm::vec4(transform.translation, 1.f);
            point_light.PushConstant().color = glm::vec4(color, pointLight.lightIntensity);
            point_light.PushConstant().radius = transform.scale.x;
            if (entity_.getComponent<ecs::RenderStateComponent>().visible) {
                graphic->draw(point_light);
            }
        }
        ecs::Entity entity_;
        [[nodiscard]] auto getChildEntitys() const -> std::vector<ecs::Entity> {
            return std::vector{point_light.entity_};
        }

    private:
        struct PointLightComponent {
                float lightIntensity = .3f;
        };
        using PointLightInstance = MeshInstance<PointLightPushConstants,
                                                render::PrimitiveTopology::Triangles, PointLightUbo>;
        PointLightInstance point_light;
        glm::vec3 color{};
        PointLightComponent pointLight;

        id_t id;
};
}  // namespace graphics::effects