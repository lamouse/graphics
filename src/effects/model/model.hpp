#pragma once
#include "effects/light/point_light.hpp"
#include "ecs/components/transform_component.hpp"
#include "world/world.hpp"
#include "render_core/graphic.hpp"
#include <tuple>
#include <tracy/Tracy.hpp>
#include <unordered_set>
namespace graphics::effects {

struct MaterialUBO {
        glm::vec4 ambient{};
        glm::vec4 diffuse{};
        glm::vec3 specular{};
        float shininess{32.f};
        AS_BYTE_SPAN
};

auto uploadMeshMaterialResource(graphics::ResourceManager& manager, const SubMesh& subMesh)
    -> std::tuple<MeshMaterialResource, MaterialUBO>;

struct ModelPushConstantData {
        glm::mat4 modelMatrix{1.f};
        glm::mat4 normalMatrix{1.f};
        AS_BYTE_SPAN
};

class LightModel {
    public:
        LightModel(graphics::ResourceManager& manager, const ModelResourceName& names,
                   const std::string& name);

        void update(const core::FrameInfo& frameInfo, world::World& world);

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
            std::vector<ecs::Entity> entity;
            entity.reserve(meshes.size());
            for (const auto& mesh : meshes) {
                entity.push_back(mesh.entity_);
            }
            return entity;
        }

         [[nodiscard]] auto getId() const -> id_t { return id; }

        ecs::Entity entity_;

    private:
        using LightMeshInstance =
            MeshInstance<ModelPushConstantData, render::PrimitiveTopology::Triangles, LightUBO,
                         MaterialUBO>;
        std::vector<LightMeshInstance> meshes;
        LightUBO light_ubo{};
        std::vector<MaterialUBO> materials;
        ModelPushConstantData push_constant;
        ecs::RenderStateComponent* render_state;
        ecs::TransformComponent* transform;
        id_t id;
        std::unordered_set<id_t> mesh_ids;
        // 用于鼠标移动
        float out_initialWorldZ{};
};

}  // namespace graphics::effects