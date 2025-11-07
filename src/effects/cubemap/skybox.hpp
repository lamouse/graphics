#pragma once
#include "resource/mesh_instance.hpp"
#include "common/common_funcs.hpp"
#include "core/frame_info.hpp"
#include "effects/effect.hpp"
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

            std::vector<std::string> cube_map_images = {
                "images/cube/sky/posx.jpg", "images/cube/sky/negx.jpg", "images/cube/sky/posy.jpg",
                "images/cube/sky/negy.jpg", "images/cube/sky/posz.jpg", "images/cube/sky/negz.jpg"};

            std::vector<std::string> cube_map_images2 = {
                "images/cube/sky2/right.jpg", "images/cube/sky2/left.jpg",
                "images/cube/sky2/top.jpg",   "images/cube/sky2/bottom.jpg",
                "images/cube/sky2/front.jpg", "images/cube/sky2/back.jpg"};

            // auto texture_id = manager.addCubeMapTexture(cube_map_images2, "skybox2");
            auto ret_id = manager.addKtxTexture("images/cube/sky2.ktx2");
            std::string mesh_path = "cube.obj";
            auto mesh_id = manager.addModel(mesh_path);
            auto sub_mesh = manager.getModelSubMesh(mesh_id);

            ASSERT_MSG(!sub_mesh.empty() || sub_mesh.size() > 1, "cube mesh count error");

            auto shader_hash = manager.addGraphShader("skycube");

            sky_box = SkyBoxInstance{render::RenderCommand{.indexOffset = sub_mesh[0].indexOffset,
                                                           .indexCount = sub_mesh[0].indexCount},
                                     shader_hash,
                                     layout,
                                     "sky box instance",
                                     mesh_id,
                                     ret_id};
            auto& pipeline_state = sky_box.entity_.getComponent<ecs::DynamicPipeStateComponenet>();
            pipeline_state.state.depthTestEnable = 1;
            pipeline_state.state.depthWriteEnable = 0;

        }

        void update(const core::FrameInfo& frameInfo) {
            glm::mat4 view = frameInfo.camera->getView();
            glm::mat4 viewNoTranslate = glm::mat4(glm::mat3(view));  // 去除平移
            sky_box.getUBO<SkyUBO>().modelMatrix = viewNoTranslate;
            sky_box.getUBO<SkyUBO>().projectionMatrix = frameInfo.camera->getProjection();
        }
        [[nodiscard]] auto getChildEntitys() const -> std::vector<ecs::Entity> {
            return std::vector{sky_box.entity_};
        }
        void draw(render::Graphic* graphic) {
            if (entity_.getComponent<ecs::RenderStateComponent>().visible) {
                graphic->draw(sky_box);
            }
        }
        ecs::Entity entity_;

    private:
        using SkyBoxInstance =
            MeshInstance<EmptyPushConstants, render::PrimitiveTopology::Triangles, SkyUBO>;
        SkyBoxInstance sky_box;
        id_t id;
};

}  // namespace graphics::effects