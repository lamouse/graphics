#pragma once
#include "resource/instance.hpp"
#include "resource/obj/particle.hpp"
namespace graphics {

class ParticleInstance : public IModelInstance {
    public:
        [[nodiscard]] auto getTextureId() const -> render::TextureId override { return {}; }
        void setTextureId(render::TextureId) override {}
        [[nodiscard]] auto getMeshId() const -> render::MeshId override { return meshId; }
        [[nodiscard]] auto getUBOData() const -> std::span<const std::byte> override { return {}; }
        render::MeshId meshId;
        [[nodiscard]] auto getPrimitiveTopology() const -> render::PrimitiveTopology override {
            return topology;
        }

    private:
        render::PrimitiveTopology topology = render::PrimitiveTopology::Points;
};

}  // namespace graphics