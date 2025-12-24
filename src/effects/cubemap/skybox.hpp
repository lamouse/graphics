#pragma once
#include "resource/mesh_instance.hpp"
#include "common/common_funcs.hpp"
#include "core/frame_info.hpp"
#include "effects/effect.hpp"
#include "world/world.hpp"
namespace graphics::effects {
struct SkyUBO {
        glm::mat4 modelMatrix{1.f};
        glm::mat4 projectionMatrix{1.f};
        AS_BYTE_SPAN
};
class SkyBox {
    public:
        CLASS_DEFAULT_MOVEABLE(SkyBox);
        CLASS_NON_COPYABLE(SkyBox);
        ~SkyBox() = default;
        SkyBox(graphics::ResourceManager& manager, const layout::FrameBufferLayout& layout)
            : id(getCurrentId()) {
            entity_ = getEffectsScene().createEntity("SkyBox" + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
            render_state = &entity_.getComponent<ecs::RenderStateComponent>();  // NOLINT
            auto ret_id = manager.addKtxCubeMap("sky.ktx2");
            std::string mesh_path = "cube.obj";
            auto mesh_id = manager.addModel(mesh_path);
            auto sub_mesh = manager.getModelSubMesh(mesh_id);

            ASSERT_MSG(!sub_mesh.empty() || sub_mesh.size() > 1, "cube mesh count error");

            auto shader_hash = manager.addGraphShader("skycube");
            MeshMaterialResource material_resource{};
            material_resource.diffuseTextures = ret_id;
            sky_box = SkyBoxInstance{render::RenderCommand{.indexOffset = sub_mesh[0].indexOffset,
                                                           .indexCount = sub_mesh[0].indexCount},
                                     shader_hash,
                                     layout,
                                     "sky box instance",
                                     mesh_id,
                                     material_resource};
            auto& pipeline_state = sky_box.entity_.getComponent<ecs::DynamicPipeStateComponenet>();
            pipeline_state.state.depthTestEnable = 1;
            pipeline_state.state.depthWriteEnable = 0;

            sky_box.setUBO<SkyUBO>(&sky_ubo);
        }

        void update(const core::FrameInfo& frameInfo, world::World& /*world*/) {
            glm::mat4 view = frameInfo.camera->getView();
            glm::mat4 viewNoTranslate = glm::mat4(glm::mat3(view));  // 去除平移
            sky_ubo.modelMatrix = viewNoTranslate;
            sky_ubo.projectionMatrix = frameInfo.camera->getProjection();
        }
        [[nodiscard]] auto getChildEntitys() const -> std::vector<ecs::Entity> {
            return std::vector{sky_box.entity_};
        }
        void draw(render::Graphic* graphic) {
            if (render_state->visible) {
                graphic->draw(sky_box);
            }
        }
        ecs::Entity entity_;

        auto getId() const -> id_t { return id; }

    private:
        using SkyBoxInstance =
            MeshInstance<EmptyPushConstants, render::PrimitiveTopology::Triangles, SkyUBO>;
        SkyUBO sky_ubo;
        SkyBoxInstance sky_box;
        ecs::RenderStateComponent* render_state{};
        id_t id;
};

}  // namespace graphics::effects