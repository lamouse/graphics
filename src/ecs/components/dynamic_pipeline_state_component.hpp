#pragma once
#include "render_core/pipeline_state.h"
#include "core/frontend/framebuffer_layout.hpp"

namespace ecs {
struct DynamicPipeStateComponenet {
        render::DynamicPipelineState state{};
        DynamicPipeStateComponenet(const layout::FrameBufferLayout& layout) {
            state.viewport.width = static_cast<float>(layout.width);
            state.viewport.height = static_cast<float>(layout.height);
            state.scissors.width = static_cast<int>(layout.width);
            state.scissors.height = static_cast<int>(layout.height);
        }
};
}  // namespace ecs