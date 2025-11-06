#pragma once
#include "effects/light/point_light.hpp"
namespace graphics::effects {
void move_model(const core::FrameInfo& frameInfo, ecs::TransformComponent& transform);
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
                meshes.emplace_back(
                    render::RenderCommand{
                        .indexOffset = mesh.indexOffset,
                        .indexCount = mesh.indexCount,
                    },
                    shader_hash, layout, name + "mesh", mesh_id, texture_id);
            }
            entity_ = getEffectsScene().createEntity("LightModel" + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
        }

        void update(const core::FrameInfo& frameInfo) {
            auto [down, first] = frameInfo.input_state.mouseLeftButtonDown();

            for (std::size_t i = 0; auto& mesh : meshes) {
                auto& transform = mesh.entity_.getComponent<ecs::TransformComponent>();
                auto& render_state = mesh.entity_.getComponent<ecs::RenderStateComponent>();
                if (render_state.mouse_select && frameInfo.input_state.mouseLeftButtonUp()) {
                    render_state.mouse_select = false;
                }
                i++;

                if (first) {
                    pending_pick_ = true;
                } else {
                    if (!render_state.mouse_select && down && pending_pick_ &&
                        entity_.getComponent<ecs::RenderStateComponent>().visible) {
                        check_pick(id, mesh.getMeshId(), frameInfo, render_state, transform);
                        if (render_state.mouse_select) {
                            pending_pick_ = false;
                        }
                    }
                    if (i == meshes.size()) {
                        pending_pick_ = false;
                    }
                }

                // 🚀 2. 如果已选中且按住左键 → 跟随鼠标移动
                if (render_state.mouse_select) {
                    move_model(frameInfo, transform);
                }
                PointLight light{};
                light.color = {1.f, 1.f, 1.f, 1.f};
                light.position = {2.0f, 2.0f, 2.f, .4};
                mesh.PushConstant().modelMatrix = transform.mat4();
                mesh.PushConstant().normalMatrix = transform.normalMatrix();
                mesh.getUBO().numLights = 6;
                mesh.getUBO().projection = frameInfo.camera->getProjection();
                mesh.getUBO().view = frameInfo.camera->getView();
                mesh.getUBO().ambientLightColor.w = .3f;
                mesh.getUBO().pointLights[0] = light;
            }
        }

        void draw(render::Graphic* graphic) {
            if (entity_.getComponent<ecs::RenderStateComponent>().visible) {
                for (auto& mesh : meshes) {
                    graphic->draw(mesh);
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
        using LightMeshInstance = MeshInstance<PointLightUbo, ModelPushConstantData,
                                               render::PrimitiveTopology::Triangles>;
        std::vector<LightMeshInstance> meshes;
        // TODO 主要修复第一次按下鼠标左键无法拾取的问题，等找到修复方案再修复
        bool pending_pick_ = false;
        id_t id;
};

}  // namespace graphics::effects