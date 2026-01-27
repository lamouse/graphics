#include "effects/effect.hpp"
#include "ecs/scene/scene.hpp"
#include "effects/model/multi_mesh_model.hpp"
#include "effects/model/model.hpp"
#include "common/file.hpp"
#include <nlohmann/json.hpp>

constexpr std::string_view MODEL_ASSET_PATH = "models";
namespace graphics::effects {
namespace {
auto serialize_model_effect_info_to_asset(const ModelEffectInfo& info) -> nlohmann::json {
    nlohmann::json j;
    nlohmann::json pipeline_state_json =
        nlohmann::json::parse(info.pipeline_state.to_json_string());
    j["source_path"] = info.source_path;
    j["model_name"] = info.model_name;
    j["shader_name"] = info.shader_name;
    j["asset_name"] = info.asset_name;
    j["flip_uv"] = info.flip_uv;
    j["use_mtl"] = info.use_mtl;
    j["copy_local"] = info.copy_local;
    j["split_mesh"] = info.split_mesh;
    j["default_scale"] = info.default_scale;
    j["pipeline_state"] = pipeline_state_json;
    return j;
}

auto deserialize_effect_info_from_asset(const nlohmann::json& j) -> ModelEffectInfo {
    ModelEffectInfo info;
    info.source_path = j.value("source_path", "");
    info.model_name = j.value("model_name", "");
    info.shader_name = j.value("shader_name", "");
    info.asset_name = j.value("asset_name", "");
    info.flip_uv = j.value("flip_uv", false);
    info.use_mtl = j.value("use_mtl", false);
    info.copy_local = j.value("copy_local", true);
    info.split_mesh = j.value("split_mesh", false);
    info.default_scale = j.value("default_scale", 1.0f);
    info.pipeline_state =
        render::DynamicPipelineState::from_json_string(j["pipeline_state"].dump());
    return info;
}
}  // namespace

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
auto load_model_form_asset(ResourceManager& manager) -> std::vector<Model> {
    std::vector<Model> models;
    auto asset_path = common::get_module_path(common::ModuleType::Asset) / MODEL_ASSET_PATH;
    if(!std::filesystem::exists(asset_path)){
        return models;
    }
    for (const auto& entry : std::filesystem::directory_iterator(asset_path)) {
        if (entry.is_regular_file()) {
            const auto& path = entry.path();
            if (path.extension() == ".json") {
                nlohmann::json json_data;
                std::ifstream f(path);
                f >> json_data;
                auto info = deserialize_effect_info_from_asset(json_data);
                models.push_back(create_model(info, manager));
            }
        }
    }

    return models;
}

void save_model_to_asset(const ModelEffectInfo& info) {
    auto json_data = serialize_model_effect_info_to_asset(info);
    auto asset_path =
        (common::get_module_path(common::ModuleType::Asset) / MODEL_ASSET_PATH) / info.asset_name;
    common::create_dir(common::get_module_path(common::ModuleType::Asset) / MODEL_ASSET_PATH);
    std::ofstream file(asset_path);
    if (file.is_open()) {
        file << json_data.dump(4);
        file.close();
    }
}

}  // namespace graphics::effects