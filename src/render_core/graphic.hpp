#pragma once
#include "render_core/fixed_pipeline_state.h"
#include "common/common_funcs.hpp"
#include "resource/model_instance.hpp"
#include <imgui.h>
namespace render {
using GraphicsId = common::SlotId;
class Graphic {
    public:
        virtual ~Graphic() = default;
        virtual void setPipelineState(const PipelineState& state) = 0;
        virtual auto getDrawImage() -> ImTextureID = 0;
        virtual auto uploadModel(const graphics::IModelInstance& instance) -> ModelId = 0;
        virtual void draw(const graphics::IModelInstance& instance) = 0;
        virtual void clean() = 0;
        virtual void end() = 0;
        Graphic() = default;
        CLASS_DEFAULT_COPYABLE(Graphic);
        CLASS_DEFAULT_MOVEABLE(Graphic);
};
}  // namespace render