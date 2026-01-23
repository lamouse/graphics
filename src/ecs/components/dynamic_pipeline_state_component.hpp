#pragma once
#include "render_core/pipeline_state.h"
#include "common/settings.hpp"

namespace ecs {
struct DynamicPipeStateComponenet {
        render::DynamicPipelineState state;
        DynamicPipeStateComponenet() {
            state.viewport.width = static_cast<float>(settings::values.resolution.width);
            state.viewport.height = static_cast<float>(settings::values.resolution.height);
            state.scissors.width = static_cast<int>(settings::values.resolution.width);
            state.scissors.height = static_cast<int>(settings::values.resolution.height);
        }
};
}  // namespace ecs