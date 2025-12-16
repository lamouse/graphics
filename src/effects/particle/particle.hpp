#pragma once
#include "render_core/compute_instance.hpp"
#include "render_core/graphic.hpp"
#include "resource/resource.hpp"
#include "resource/obj/particle.hpp"
#include "resource/mesh_instance.hpp"
#include "resource/id.hpp"
#include "ecs/scene/entity.hpp"
#include "ecs/components/render_state_component.hpp"
#include "effects/effect.hpp"
#include "core/frame_info.hpp"
#include "common/common_funcs.hpp"
#include "world/world.hpp"
const uint32_t PARTICLE_COUNT = 8192;

constexpr const char* particle_shader_name = "particle";
namespace graphics::effects {

enum class ParticleType : std::uint8_t { Delta };

inline auto getParticleTypeName(ParticleType type) -> const char* {
    switch (type) {
        case ParticleType::Delta:
            return "Delta";
    }
    return "unknown";
}

template <typename UBO, ParticleType Type>
    requires ByteSpanConvertible<UBO> && std::is_trivially_copyable_v<UBO>
class Particle : public render::IComputeInstance {
    public:
        CLASS_DEFAULT_MOVEABLE(Particle);
        CLASS_NON_COPYABLE(Particle);

        [[nodiscard]] auto getUBOData() const -> std::vector<std::span<const std::byte>> override {
            return std::vector<std::span<const std::byte>>{u.as_byte_span()};
        };
        Particle(graphics::ResourceManager& manager, const layout::FrameBufferLayout& layout,
                 std::uint32_t count)
            : id(getCurrentId()) {
            graphics::ParticleModel model{
                count, static_cast<float>(layout.width) / static_cast<float>(layout.height)};
            auto in_mesh = manager.addMesh("particle_in", model);
            auto out_mesh = manager.addMesh("particle_out", model);

            auto shader_hash = manager.getShaderHash<ShaderHash>(particle_shader_name);
            // NOLINTNEXTLINE
            in = DeltaParticleInstance({}, shader_hash, layout, "particle", in_mesh);
            // NOLINTNEXTLINE
            out = DeltaParticleInstance({}, shader_hash, layout, "particle", out_mesh);
            compute_mesh.at(0) = out.getMeshId();
            compute_mesh.at(1) = in.getMeshId();
            mesh_ids = compute_mesh;
            entity_ =
                getEffectsScene().createEntity(getParticleTypeName(Type) + std::to_string(id));
            entity_.addComponent<ecs::RenderStateComponent>(id);
            compute_shader_hash = manager.getShaderHash<std::uint64_t>(particle_shader_name);
            uint32_t group_x = (count + LOCAL_SIZE - 1) / LOCAL_SIZE;
            workgroup_size = {group_x, 1, 1};
        }

        void update(const core::FrameInfo& frame, world::World& world) {}
        void draw(render::Graphic* graphics) {
            graphics->dispatchCompute(*this);
            std::swap(compute_mesh.at(0), compute_mesh.at(1));
            if (entity_.getComponent<ecs::RenderStateComponent>().visible) {
                if (draw_out) {
                    graphics->draw(out);
                    draw_out = false;
                } else {
                    graphics->draw(in);
                    draw_out = true;
                }
            }
        }

        auto getUniforBuffer() -> UBO& { return u; }
        auto getChildEntitys() -> std::vector<ecs::Entity> {
            return std::vector{in.entity_, out.entity_};
        }
        ecs::Entity entity_;

    private:
        id_t id;
        using DeltaParticleInstance =
            MeshInstance<EmptyPushConstants, render::PrimitiveTopology::Points, EmptyUnformBuffer>;

        DeltaParticleInstance in;
        DeltaParticleInstance out;
        UBO u;
        std::array<render::MeshId, 2> compute_mesh;
        bool draw_out{true};
        inline static ecs::Scene scene;
        static constexpr uint32_t LOCAL_SIZE = 256;
};

struct DeltaUniformBufferObject {
        float deltaTime = 1.0f;
        [[nodiscard]] auto as_byte_span() const -> std::span<const std::byte> {
            return std::span{reinterpret_cast<const std::byte*>(this), sizeof(*this)};
        }
};

// 在头文件或命名空间中定义
using DeltaParticle = Particle<DeltaUniformBufferObject, ParticleType::Delta>;

}  // namespace graphics::effects