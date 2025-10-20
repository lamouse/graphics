#pragma once
#include "common/common_funcs.hpp"
#include "render_core/types.hpp"
#include <span>
#include "render_core/fixed_pipeline_state.h"
namespace graphics {
class IModelInstance {
    public:
        IModelInstance() = default;
        virtual ~IModelInstance() = default;
        CLASS_DEFAULT_COPYABLE(IModelInstance);
        CLASS_DEFAULT_MOVEABLE(IModelInstance);
        [[nodiscard]] virtual auto getTextureId() const -> render::TextureId = 0;
        virtual void setTextureId(render::TextureId) = 0;
        [[nodiscard]] virtual auto getMeshId() const -> render::MeshId = 0;
        [[nodiscard]] virtual auto getUBOData() const -> std::span<const std::byte> = 0;

        [[nodiscard]] virtual auto getPrimitiveTopology() const -> render::PrimitiveTopology = 0;
};
}  // namespace graphics