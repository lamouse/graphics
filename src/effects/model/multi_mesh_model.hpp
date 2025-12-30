#pragma once
#include "effects/model/model.hpp"
#include "render_core/graphic.hpp"

namespace graphics::effects {
class ModelForMultiMesh {
    public:
        ModelForMultiMesh(ResourceManager& manager,
                          const ModelResourceName& names, const std::string& name);
        ecs::Entity entity_;
        void draw(render::Graphic* graphic) {
            ZoneScopedNC("model::draw", 210);
            if (render_state->visible) {
                for (auto& mesh : meshes) {
                    if (mesh.render_state->visible) {
                        graphic->draw(mesh);
                    }
                }
            }
        }

        [[nodiscard]] auto getChildEntitys() const -> std::vector<ecs::Entity> {
            return child_entitys_;
        }

        void update(const core::FrameInfo& frameInfo, world::World& world);
        [[nodiscard]] auto getId() const -> id_t { return id; }

    private:
        using MeshInstance =
            MeshInstance<ModelPushConstantData, render::PrimitiveTopology::Triangles, LightUBO,
                         MaterialUBO>;
        std::vector<MeshInstance> meshes;
        id_t id;

        LightUBO light_ubo{};
        std::vector<MaterialUBO> materials;
        // TODO 主要修复第一次按下鼠标左键无法拾取的问题，等找到修复方案再修复
        bool pending_pick_ = false;
        // 用于鼠标移动
        glm::vec3 out_dragStartWorldPos{};
        float out_initialWorldZ{};
        ModelPushConstantData push_constant;
        std::unordered_set<id_t> mesh_ids;
        ecs::RenderStateComponent* render_state{nullptr};
        ecs::TransformComponent* transform{nullptr};
        std::vector<ecs::Entity> child_entitys_;
};
}  // namespace graphics::effects