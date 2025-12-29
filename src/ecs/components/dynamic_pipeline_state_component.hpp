#pragma once
#include "render_core/pipeline_state.h"
#include "core/frontend/framebuffer_layout.hpp"
#include "common/settings.hpp"

namespace ecs {
struct DynamicPipeStateComponenet {
        render::DynamicPipelineState state{};
        DynamicPipeStateComponenet(const layout::FrameBufferLayout& layout) {
            state.viewport.width = static_cast<float>(settings::values.resolution.weight);
            state.viewport.height = static_cast<float>(settings::values.resolution.height);
            state.scissors.width = static_cast<int>(settings::values.resolution.weight);
            state.scissors.height = static_cast<int>(settings::values.resolution.height);
        }
};
}  // namespace ecs