#pragma once
#include "effects/light/point_light.hpp"
namespace graphics::effects {
struct ModelPushConstantData {
        glm::mat4 modelMatrix{1.f};
        glm::mat4 normalMatrix{1.f};
        AS_BYTE_SPAN
};

class LightModel {
    public:
        LightModel(graphics::ResourceManager& manager, const layout::FrameBufferLayout& layout, const ModelResourceName& names,
                   const std::string& name)
            : model_(manager, layout, names, "LightModel"), id(getCurrentId()) {
            entity_ = getEffectsScene().createEntity("LightModel" + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
        }

        void update(const core::FrameInfo& frameInfo) {
            auto& transform = model_.entity_.getComponent<ecs::TransformComponent>();
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

        [[nodiscard]] auto getChildEntitys() const ->std::vector<ecs::Entity>{
            return std::vector{model_.entity_};
        }

        ecs::Entity entity_;

    private:
        using LightModelInstance = ModelInstance<PointLightUbo, ModelPushConstantData,
                                                 render::PrimitiveTopology::Triangles>;
        LightModelInstance model_;
        id_t id;
};

}  // namespace graphics::effects