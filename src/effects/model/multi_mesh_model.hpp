#pragma once
#include "effects/model/model.hpp"

namespace graphics::effects {
class ModelForMultiMesh {
    public:
        ModelForMultiMesh(ResourceManager& manager, const layout::FrameBufferLayout& layout,
                          const ModelResourceName& names, const std::string& name);
        ecs::Entity entity_;
        void draw(render::Graphic* graphic) {
            ZoneScopedNC("model::draw", 210);
            if (entity_.getComponent<ecs::RenderStateComponent>().visible) {
                for (auto& mesh : meshes) {
                    if (mesh.entity_.getComponent<ecs::RenderStateComponent>().visible) {
                        graphic->draw(mesh);
                    }
                }
            }
        }

        [[nodiscard]] auto getChildEntitys() const -> std::vector<ecs::Entity> {
            std::vector<ecs::Entity> entity;
            entity.reserve(meshes.size());
            for (const auto& mesh : meshes) {
                entity.push_back(mesh.entity_);
            }
            return entity;
        }

        void update(const core::FrameInfo& frameInfo, world::World& world) {
            ZoneScopedNC("model::update", 110);
            const auto [down, first] = frameInfo.input_state.mouseLeftButtonDown();

            auto& transform = entity_.getComponent<ecs::TransformComponent>();
            auto& render_state = entity_.getComponent<ecs::RenderStateComponent>();

            // transform.rotation.x = 4.8f;

            if (render_state.mouse_select && frameInfo.input_state.mouseLeftButtonUp()) {
                render_state.mouse_select = false;
            }
            if (render_state.mouse_select) {
                // move_model(frameInfo, transform);
                move_model(frameInfo, transform, false, out_initialWorldZ, out_dragStartWorldPos);
            }
            if (render_state.is_select()) {
                if (frameInfo.input_state.key == core::InputKey::LCtrl) {
                    if (frameInfo.input_state.scrollOffset_ != 0.0f) {
                        auto scale = transform.scale.x;
                        scale += frameInfo.input_state.scrollOffset_ * 0.02f;
                        if (scale <= 0.01) {
                            scale = 0.01f;
                        }
                        if (scale > 2.0) {
                            scale = 2.0f;
                        }
                        transform.scale = glm::vec3(scale);
                    }
                }
            }

            if (first) {
                pending_pick_ = true;
            } else {
                if (!render_state.mouse_select && down && pending_pick_ &&
                    entity_.getComponent<ecs::RenderStateComponent>().visible) {
                    check_pick(id, meshes[0].getMeshId(), frameInfo, render_state, transform);
                }
                if (render_state.mouse_select) {
                    move_model(frameInfo, transform, true, out_initialWorldZ,
                               out_dragStartWorldPos);
                }
                pending_pick_ = false;
            }

            auto light_entity = world.getLightEntities();
            LightUBO pointLightUbo{};
            pointLightUbo.projection = frameInfo.camera->getProjection();
            pointLightUbo.view = frameInfo.camera->getView();
            pointLightUbo.inverseView = frameInfo.camera->getView();
            pointLightUbo.ambientLightColor = glm::vec4{1.f, 1.f, 1.f, .04f};
            int index = 0;
            for (const auto& entity : light_entity) {
                auto& lightComponent = entity.getComponent<ecs::LightComponent>();
                if (lightComponent.type == ecs::LightType::Point) {
                    auto& light_transform = entity.getComponent<ecs::TransformComponent>();
                    PointLight light{};

                    light.color = {lightComponent.color, lightComponent.intensity};
                    light.position = {light_transform.translation, 1.0f};
                    pointLightUbo.pointLights[index] = light;
                    index++;
                    if (index >= MAX_LIGHTS) {
                        break;
                    }
                } else if (lightComponent.type == ecs::LightType::Directional) {
                    DirLight dirLight{};
                    dirLight.direction = glm::vec4(glm::normalize(lightComponent.direction), 0.f);
                    dirLight.color =
                        glm::vec4(lightComponent.color, 0.f) * lightComponent.intensity;
                    pointLightUbo.dirLight = dirLight;

                    // TODO 临时测试
                    pointLightUbo.spotLight.position =
                        glm::vec4(frameInfo.camera->getPosition(), lightComponent.outerCone);
                    pointLightUbo.spotLight.direction =
                        glm::vec4(frameInfo.camera->front(), lightComponent.innerCone);
                    pointLightUbo.spotLight.color = glm::vec4(lightComponent.color, 10);
                    // TODO 临时测试
                }
            }
            auto modelMatrix = transform.mat4();
            auto normalMatrix = transform.normalMatrix();
            pointLightUbo.numLights = index;
            for (auto& mesh : meshes) {
                mesh.PushConstant().modelMatrix = modelMatrix;
                mesh.PushConstant().normalMatrix = normalMatrix;
                mesh.getUBO<LightUBO>() = pointLightUbo;
            }
        }

    private:
        using MeshInstance =
            MeshInstance<ModelPushConstantData, render::PrimitiveTopology::Triangles, LightUBO,
                         MaterialUBO>;
        std::vector<MeshInstance> meshes;
        id_t id;

        // TODO 主要修复第一次按下鼠标左键无法拾取的问题，等找到修复方案再修复
        bool pending_pick_ = false;
        // 用于鼠标移动
        glm::vec3 out_dragStartWorldPos;
        float out_initialWorldZ;
};
}  // namespace graphics::effects