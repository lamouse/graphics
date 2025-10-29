#pragma once
#include "resource/instance.hpp"
#include "common/common_funcs.hpp"
#include "id.hpp"
namespace graphics {
class ResourceManager;
class ParticleInstance : public IModelInstance {
    public:
        [[nodiscard]] auto getUBOData() const -> std::span<const std::byte> override { return {}; }
        [[nodiscard]] auto getPrimitiveTopology() const -> render::PrimitiveTopology override {
            return topology;
        }
        explicit ParticleInstance(const ResourceManager& resource, const std::string& textureName,
                                  const std::string& meshName, std::string shaderName);
        CLASS_DEFAULT_COPYABLE(ParticleInstance);
        CLASS_DEFAULT_MOVEABLE(ParticleInstance);
        ~ParticleInstance() override = default;
        ParticleInstance() = default;
    private:
        render::PrimitiveTopology topology = render::PrimitiveTopology::Points;
        id_t id;
        std::string texture;
        std::string mesh;
        std::string shader;
};

}  // namespace graphics