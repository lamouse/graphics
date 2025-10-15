#pragma once
#include "render_core/fixed_pipeline_state.h"
#include "render_core/types.hpp"
#include "common/common_funcs.hpp"
#include "resource/model_instance.hpp"
#include "resource/texture/image.hpp"
#include "resource/obj/mesh.hpp"
#include <imgui.h>
namespace render {
using GraphicsId = common::SlotId;
class Graphic {
    public:
        virtual ~Graphic() = default;
        virtual void setPipelineState(const PipelineState& state) = 0;
        virtual auto getDrawImage() -> ImTextureID = 0;
        virtual auto uploadModel(const graphics::IMeshData& instance) -> MeshId = 0;
        virtual auto uploadTexture(const ::resource::image::ITexture& texture ) ->TextureId = 0;
        virtual void draw(const graphics::IModelInstance& instance) = 0;
        virtual void clean() = 0;
        virtual void end() = 0;
        Graphic() = default;
        CLASS_DEFAULT_COPYABLE(Graphic);
        CLASS_DEFAULT_MOVEABLE(Graphic);
};
}  // namespace render