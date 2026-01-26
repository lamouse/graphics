#include "effects/effect.hpp"
#include "ecs/scene/scene.hpp"
#include "effects/model/multi_mesh_model.hpp"
#include "effects/model/model.hpp"
namespace graphics::effects {
auto getEffectsScene() -> ecs::Scene& {
    static ecs::Scene instance;
    return instance;
}

auto create_model(const ModelEffectInfo& info, ResourceManager& manager) -> Model {
    if (info.split_mesh) {
        ModelResourceName names{
            .shader_name = info.shader_name,
            .mesh_name = info.model_name,
        };
        return std::make_shared<graphics::effects::ModelForMultiMesh>(manager, names,
                                                                      info.model_name);
    }
    ModelResourceName names{
        .shader_name = info.shader_name,
        .mesh_name = info.model_name,
    };
    return std::make_shared<graphics::effects::LightModel>(manager, names, info.model_name);
}

}  // namespace graphics::effects