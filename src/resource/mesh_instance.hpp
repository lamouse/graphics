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
        std::string texture_name;
};

template <typename PushConstants, render::PrimitiveTopology primitiveTopology, typename... UBO>
    requires ByteSpanConvertible<PushConstants> && std::is_trivially_copyable_v<PushConstants> &&
             (ByteSpanConvertible<UBO> && ...) && (std::is_trivially_copyable_v<UBO> && ...)
class MeshInstance : public IMeshInstance {
    public:
        using UBOs = std::tuple<UBO...>;

        CLASS_DEFAULT_MOVEABLE(MeshInstance);
        CLASS_NON_COPYABLE(MeshInstance);
        [[nodiscard]] auto getEntity() -> ecs::Entity& { return entity_; }
        ~MeshInstance() override = default;
        [[nodiscard]] auto getPushConstants() const -> std::span<const std::byte> override {
            return push_constants.as_byte_span();
        };
        [[nodiscard]] auto getPipelineState() const -> render::DynamicPipelineState override {
            return entity_.getComponent<ecs::DynamicPipeStateComponenet>().state;
        }
        [[nodiscard]] auto getMaterialIds() const -> std::vector<render::TextureId> override {
            return {material_resource.ambientTextures, material_resource.diffuseTextures,
                    material_resource.specularTextures, material_resource.normalTextures};
        }
        auto getUBO() -> UBOs& { return ubos; }

        template <std::size_t N>
        auto getUBO() -> std::tuple_element_t<N, std::tuple<UBO...>>& {
            return std::get<N>(ubos);
        }

        template <std::size_t N>
        auto getUBO() const -> const std::tuple_element_t<N, std::tuple<UBO...>>& {
            return std::get<N>(ubos);
        }

        template <std::size_t N, typename T = std::tuple_element_t<N, std::tuple<UBO...>>>
        void setUBO(const T& value) {
            std::get<N>(ubos) = value;
        }

        // ✅ 根据类型获取 UBO（可读写）
        template <typename T>
        auto getUBO() -> T& {
            static_assert((std::is_same_v<T, UBO> || ...), "T is not one of the UBO types");
            return std::get<T>(ubos);
        }

        // ✅ const 版本
        template <typename T>
        auto getUBO() const -> const T& {
            static_assert((std::is_same_v<T, UBO> || ...), "T is not one of the UBO types");
            return std::get<T>(ubos);
        }

        // ✅ 设置 UBO（通过类型）
        template <typename T>
        void setUBO(const T& value) {
            static_assert((std::is_same_v<T, UBO> || ...), "T is not one of the UBO types");
            std::get<T>(ubos) = value;
        }

        // === 获取所有 UBO 的 byte spans（用于渲染）===
        [[nodiscard]] auto getUBOs() const -> std::vector<std::span<const std::byte>> override {
            return std::apply(
                [](const auto&... ubo) -> auto {
                    return std::vector<std::span<const std::byte>>{(ubo).as_byte_span()...};
                },
                ubos);
        }

        auto PushConstant() -> PushConstants& { return push_constants; }

        MeshInstance(render::RenderCommand render_command_, ShaderHash shaderHash_,
                     const layout::FrameBufferLayout& layout, const std::string& meshName = "",
                     render::MeshId meshId_ = {}, MeshMaterialResource material_resource_ = {})
            : IMeshInstance(primitiveTopology, render_command_, meshId_, shaderHash_.vertex,
                            shaderHash_.fragment),
              material_resource(material_resource_) {
            entity_ = getModelScene().createEntity(meshName.empty()
                                                       ? "Mesh " + std::to_string(id)
                                                       : meshName + " " + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
            entity_.addComponent<ecs::DynamicPipeStateComponenet>(layout);
        }
        MeshInstance() = default;

    private:
        UBOs ubos{};
        PushConstants push_constants;
        MeshMaterialResource material_resource;
};

}  // namespace graphics
