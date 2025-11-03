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
            : model_(manager, layout, names, "LightModel"), id(getCurrentId()) {
            entity_ = getEffectsScene().createEntity("LightModel" + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
        }

        void update(const core::FrameInfo& frameInfo) {
            auto& transform = model_.entity_.getComponent<ecs::TransformComponent>();
            auto& render_state = model_.entity_.getComponent<ecs::RenderStateComponent>();

            if (render_state.mouse_select && frameInfo.input_state.mouseLeftButtonUp()) {
                render_state.mouse_select = false;
            }
            auto [down, first] = frameInfo.input_state.mouseLeftButtonDown();
            if (first) {
                pending_pick_ = true;
            } else {
                if (!render_state.mouse_select && down) {
                    check_pick(model_.getId(), model_.getMeshId(), frameInfo, render_state,
                               transform);
                }
                pending_pick_ = false;
            }
            // 🚀 2. 如果已选中且按住左键 → 跟随鼠标移动
            if (render_state.mouse_select) {
                move_model(frameInfo, transform);
            }
            model_.PushConstant().modelMatrix = transform.mat4();
            model_.PushConstant().normalMatrix = transform.normalMatrix();
            model_.getUBO().numLights = 1;
            model_.getUBO().projection = frameInfo.camera->getProjection();
            model_.getUBO().view = frameInfo.camera->getView();
            model_.getUBO().ambientLightColor.w = 1.f;
            model_.getUBO().pointLights[0].position = glm::vec4(1.0, 0.5, 0.3, 1.f);
        }

        void draw(render::Graphic* graphic) {
            if (entity_.getComponent<ecs::RenderStateComponent>().visible) {
                graphic->draw(model_);
            }
        }

        [[nodiscard]] auto getChildEntitys() const -> std::vector<ecs::Entity> {
            return std::vector{model_.entity_};
        }

        ecs::Entity entity_;

    private:
        using LightModelInstance = ModelInstance<PointLightUbo, ModelPushConstantData,
                                                 render::PrimitiveTopology::Triangles>;
        LightModelInstance model_;
        // TODO 主要修复第一次按下鼠标左键无法拾取的问题，等找到修复方案再修复
        bool pending_pick_ = false;
        id_t id;
};

}  // namespace graphics::effects