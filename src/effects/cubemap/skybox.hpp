#pragma once
#include "resource/model_instance.hpp"
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
        SkyBox(graphics::ResourceManager& manager) : id(getCurrentId()) {
            entity_ = getEffectsScene().createEntity("SkyBox" + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);

            std::vector<std::string> cube_map_images = {
                "images/cube/sky/posx.jpg", "images/cube/sky/negx.jpg", "images/cube/sky/posy.jpg",
                "images/cube/sky/negy.jpg", "images/cube/sky/posz.jpg", "images/cube/sky/negz.jpg"};

            manager.addTexture(cube_map_images, "skybox");
            std::string mesh_path = "models/cube.obj";
            manager.addMesh(mesh_path);
            manager.addGraphShader("skycube");
            ModelResourceName names{.shader_name = "skycube",
                                    .mesh_name = mesh_path,
                                    .texture_name = "skybox"};
                                    sky_box = SkyBoxInstance{manager, names, "skybox instance"};
        }

        void update(core::FrameInfo& frameInfo) {
            auto& transform = sky_box.entity_.getComponent<ecs::TransformComponent>();
            sky_box.getUBO().modelMatrix = transform.mat4();
            sky_box.getUBO().projectionMatrix = glm::mat4(1.f);
        }

        void draw(render::Graphic* graphic) {
            if (entity_.getComponent<ecs::RenderStateComponent>().visible) {
                graphic->draw(sky_box);
            }
        }
        ecs::Entity entity_;

    private:
        using SkyBoxInstance =
            ModelInstance<SkyUBO, EmptyPushConstants, render::PrimitiveTopology::Triangles>;
        SkyBoxInstance sky_box;
        id_t id;
};

}  // namespace graphics::effects