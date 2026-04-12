#pragma once
#include "resource/resource.hpp"
#include "resource/mesh_instance.hpp"
#include "render_core/pipeline_state.h"
#include <string>
#include <memory>
#include <variant>
#include <generator>
#include <vector>
#include <algorithm>

namespace ecs {
class Scene;
}
namespace graphics::effects {

enum class EffectType { Model, Light, CubeMap, Particle };

class LightModel;
class ModelForMultiMesh;

struct ModelEffectInfo {
        std::string source_path;
        std::string model_name;
        std::string shader_name;
        std::string asset_name;
        render::DynamicPipelineState pipeline_state;
        bool flip_uv{false};
        bool use_mtl{false};
        bool copy_local{true};
        bool split_mesh{false};
        float default_scale{1.f};
};

using Model = std::variant<std::shared_ptr<graphics::effects::LightModel>,
                           std::shared_ptr<graphics::effects::ModelForMultiMesh>>;

auto create_model(const ModelEffectInfo& info, ResourceManager& manager) -> Model;

void save_model_to_asset(const ModelEffectInfo& info);

auto load_model_form_asset(ResourceManager& manager) -> std::vector<Model>;

auto getEffectsScene() -> ecs::Scene&;

struct Effect {
        std::generator<std::monostate> coroutine_;
        decltype(coroutine_.begin()) iter_;
        explicit Effect(std::generator<std::monostate> coroutine)
            : coroutine_(std::move(coroutine)), iter_(coroutine_.begin()) {}
        auto update() -> bool {
            if (iter_ == coroutine_.end()) {
                return false;
            }
            ++iter_;
            return true;
        }
};

class EffectManager {
    public:
        void add(Effect effect) { effects_.emplace_back(std::move(effect)); }

        void run() {
            std::erase_if(effects_, [](Effect& effect)-> bool { return !effect.update(); });
        }

    private:
        std::vector<Effect> effects_;
};

}  // namespace graphics::effects