#pragma once
#include "effects/light/point_light.hpp"
#include "ecs/components/transform_component.hpp"
#include "world/world.hpp"
namespace graphics::effects {
void move_model(const core::FrameInfo& frameInfo, ecs::TransformComponent& transform);
void move_model(const core::FrameInfo& frameInfo, ecs::TransformComponent& transform,
                bool startDragThisFrame,
                float& out_initialWorldZ,         // 👈 只需记录一个 Z 值（更轻量）
                glm::vec3& out_dragStartWorldPos  // 仍需要完整位置用于 delta 计算
);
void check_pick(id_t id, render::MeshId meshId, const core::FrameInfo& frameInfo,
                ecs::RenderStateComponent& render_state, ecs::TransformComponent& transform);
struct ModelPushConstantData {
        glm::mat4 modelMatrix{1.f};
        glm::mat4 normalMatrix{1.f};
        AS_BYTE_SPAN
};

class LightModel {
    public:
        LightModel(graphics::ResourceManager& manager, const layout::FrameBufferLayout& layout,
                   const ModelResourceName& names, const std::string& name)
            : id(getCurrentId()) {
            auto shader_hash = manager.getShaderHash<ShaderHash>(names.shader_name);
            auto mesh_id = manager.getMesh(names.mesh_name);
            auto sub_mesh = manager.getModelSubMesh(mesh_id);
            auto texture_id = manager.getTexture(names.texture_name);
            meshes.reserve(sub_mesh.size());
            for (const auto& mesh : sub_mesh) {
                if (!mesh.material.diffuseTextures.empty()) {
                    texture_id = manager.addTexture("./images/" + mesh.material.diffuseTextures[0]);
                }
                meshes.emplace_back(
                    render::RenderCommand{
                        .indexOffset = mesh.indexOffset,
                        .indexCount = mesh.indexCount,
                    },
                    shader_hash, layout, name + "mesh", mesh_id, texture_id);
            }
            entity_ = getEffectsScene().createEntity("LightModel" + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
            entity_.addComponent<ecs::TransformComponent>();
        }

        void update(const core::FrameInfo& frameInfo, world::World& world) {
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

            auto modelMatrix = transform.mat4();
            auto normalMatrix = transform.normalMatrix();

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
            PointLightUbo pointLightUbo{};
            pointLightUbo.projection = frameInfo.camera->getProjection();
            pointLightUbo.view = frameInfo.camera->getView();
            for (int index = 0; const auto& entity : light_entity) {
                PointLight light{};
                auto& lightComponent = entity.getComponent<ecs::LightComponent>();
                auto& light_transform = entity.getComponent<ecs::TransformComponent>();
                light.color = {lightComponent.color, lightComponent.intensity};
                light.position = {light_transform.translation, 1.0f};
                pointLightUbo.numLights = index + 1;
                pointLightUbo.pointLights[index] = light;
                index++;
                if (index >= MAX_LIGHTS) {
                    break;
                }
            }
            for (auto& mesh : meshes) {
                mesh.PushConstant().modelMatrix = modelMatrix;
                mesh.PushConstant().normalMatrix = normalMatrix;
                mesh.getUBO<PointLightUbo>().projection = pointLightUbo.projection;
                mesh.getUBO<PointLightUbo>().view = pointLightUbo.view;
                for (int i = 0; const auto& light : pointLightUbo.pointLights) {
                    mesh.getUBO<PointLightUbo>().pointLights[i] = light;
                    i++;
                }
                mesh.getUBO<PointLightUbo>().numLights = pointLightUbo.numLights;
            }
        }

        void draw(render::Graphic* graphic) {
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

        ecs::Entity entity_;

    private:
        using LightMeshInstance = MeshInstance<ModelPushConstantData,
                                               render::PrimitiveTopology::Triangles, PointLightUbo>;
        std::vector<LightMeshInstance> meshes;
        // TODO 主要修复第一次按下鼠标左键无法拾取的问题，等找到修复方案再修复
        bool pending_pick_ = false;
        id_t id;

        // 用于鼠标移动
        glm::vec3 out_dragStartWorldPos;
        float out_initialWorldZ;
};

}  // namespace graphics::effects