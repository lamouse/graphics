#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include "ecs/scene/entity.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/components/render_state_component.hpp"
#include "ecs/components/dynamic_pipeline_state_component.hpp"
#include "resource/instance.hpp"
#include "resource/resource.hpp"
#include <type_traits>
#include <tuple>

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
};

template <typename PushConstants, render::PrimitiveTopology primitiveTopology, typename... UBO>
    requires ByteSpanConvertible<PushConstants> && std::is_trivially_copyable_v<PushConstants> &&
             (ByteSpanConvertible<UBO> && ...) && (std::is_trivially_copyable_v<UBO> && ...)
class MeshInstance : public render::IMeshInstance {
    public:
        using UBOs = std::tuple<UBO*...>;

        CLASS_DEFAULT_MOVEABLE(MeshInstance);
        CLASS_NON_COPYABLE(MeshInstance);
        [[nodiscard]] auto getEntity() -> ecs::Entity& { return entity_; }
        ~MeshInstance() override = default;
        [[nodiscard]] auto getPushConstants() const -> std::span<const std::byte> override {
            if (push_constants) {
                return push_constants->as_byte_span();
            }
            return {};
        };
        [[nodiscard]] auto getPipelineState() const -> render::DynamicPipelineState override {
            return *pipeline_state;
        }
        [[nodiscard]] auto getMaterialIds() const -> std::span<const render::TextureId> override {
            return materials;
        }

        // ✅ 设置 UBO（通过类型）
        template <typename T>
        void setUBO(T* value) {
            static_assert((std::is_same_v<T, UBO> || ...), "T is not one of the UBO types");
            std::get<T*>(ubos) = value;
        }

        // === 获取所有 UBO 的 byte spans（用于渲染）===
        [[nodiscard]] auto getUBOs() const -> std::span<std::span<const std::byte>> override {
            std::size_t i = 0;
            std::apply(
                [&](const auto&... ubo) -> auto { ((ubosSpans[i++] = ubo->as_byte_span()), ...); },
                ubos);
            return ubosSpans;
        }

        void updateMaterials(const MeshMaterialResource& material) {
            material_resource = material;
            materials = {material_resource.ambientTextures, material_resource.diffuseTextures,
                         material_resource.specularTextures, material_resource.normalTextures};
        }

        void setPushConstant(PushConstants* push) { push_constants = push; }

        MeshInstance(render::RenderCommand render_command_, ShaderHash shaderHash_,
                     const std::string& meshName = "", render::MeshId meshId_ = {},
                     MeshMaterialResource material_resource_ = {})
            : IMeshInstance(primitiveTopology, render_command_, meshId_, shaderHash_.vertex,
                            shaderHash_.fragment),
              material_resource(material_resource_),
              materials({material_resource.ambientTextures, material_resource.diffuseTextures,
                         material_resource.specularTextures, material_resource.normalTextures}),
              id(getCurrentId()) {
            entity_ = getModelScene().createEntity(meshName.empty()
                                                       ? "Mesh " + std::to_string(id)
                                                       : meshName + " " + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
            entity_.addComponent<ecs::DynamicPipeStateComponenet>();
            pipeline_state =                                                     // NOLINT
                &entity_.getComponent<ecs::DynamicPipeStateComponenet>().state;  // NOLINT
            render_state = &entity_.getComponent<ecs::RenderStateComponent>();   // NOLINT
        }
        [[nodiscard]] auto getId() const -> id_t { return id; }

        MeshInstance() = default;
        ecs::Entity entity_;
        ecs::RenderStateComponent* render_state;

    private:
        UBOs ubos{};
        PushConstants* push_constants{nullptr};
        MeshMaterialResource material_resource;
        render::DynamicPipelineState* pipeline_state{nullptr};
        std::vector<render::TextureId> materials;
        mutable std::vector<std::span<const std::byte>> ubosSpans{
            std::tuple_size_v<decltype(ubos)>};
        id_t id;
};

}  // namespace graphics
