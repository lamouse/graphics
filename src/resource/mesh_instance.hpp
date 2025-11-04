#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include "ecs/scene/entity.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/components/render_state_component.hpp"
#include "ecs/components/camera_component.hpp"
#include "ecs/components/dynamic_pipeline_state_component.hpp"
#include "resource/obj/model.hpp"
#include "resource/instance.hpp"
#include "resource/id.hpp"
#include "resource/resource.hpp"
#include <type_traits>

namespace graphics {
class ResourceManager;

struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        // 转换为 std::span<std::byte>
        auto as_byte_span() -> std::span<const std::byte> {
            return std::span<const std::byte>{reinterpret_cast<const std::byte*>(this),
                                              sizeof(UniformBufferObject)};
        }

        [[nodiscard]] auto as_byte_span() const -> std::span<const std::byte> {
            return std::span<const std::byte>{reinterpret_cast<const std::byte*>(this),
                                              sizeof(UniformBufferObject)};
        }
};

struct ModelResourceName {
        std::string shader_name;
        std::string mesh_name;
        std::string texture_name;
};

template <typename UBO, typename PushConstants, render::PrimitiveTopology primitiveTopology>
    requires ByteSpanConvertible<UBO> && std::is_trivially_copyable_v<UBO> &&
             ByteSpanConvertible<PushConstants> && std::is_trivially_copyable_v<PushConstants>
class MeshInstance : public IMeshInstance {
    public:
        CLASS_DEFAULT_MOVEABLE(MeshInstance);
        CLASS_NON_COPYABLE(MeshInstance);
        [[nodiscard]] auto getEntity() -> ecs::Entity& { return entity_; }
        ~MeshInstance() override = default;
        [[nodiscard]] auto getUBOData() const -> std::span<const std::byte> override {
            return ubo.as_byte_span();
        };
        [[nodiscard]] auto getPushConstants() const -> std::span<const std::byte> override {
            return push_constants.as_byte_span();
        };
        [[nodiscard]] auto getPrimitiveTopology() const -> render::PrimitiveTopology override {
            return topology;
        }
        [[nodiscard]] auto getPipelineState() const -> render::DynamicPipelineState override {
            return entity_.getComponent<ecs::DynamicPipeStateComponenet>().state;
        }
        auto getUBO() -> UBO& { return ubo; }
        auto PushConstant() -> PushConstants& { return push_constants; }

        MeshInstance(ShaderHash shaderHash_, const layout::FrameBufferLayout& layout,
                     const std::string& meshName = "", render::MeshId meshId_ = {},
                     render::TextureId textureId_ = {})
            : IMeshInstance(meshId_, textureId_, shaderHash_.vertex, shaderHash_.fragment) {
            entity_ = getModelScene().createEntity(meshName.empty()
                                                       ? "Mesh " + std::to_string(id)
                                                       : meshName + " " + std::to_string(id));
            entity_.addComponent<ecs::TransformComponent>();
            entity_.addComponent<ecs::RenderStateComponent>(id);
            entity_.addComponent<ecs::DynamicPipeStateComponenet>(layout);
        }
        MeshInstance() = default;

    private:
        UBO ubo{};
        PushConstants push_constants;
        render::PrimitiveTopology topology = primitiveTopology;
};

}  // namespace graphics
