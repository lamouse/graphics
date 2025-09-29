#pragma once
#include "render_core/vertex.hpp"
#include "render_core/fixed_pipeline_state.h"
#include <span>

namespace ecs {

    struct ModelCompone{
        std::vector<render::VertexAttribute> attrs;
    };

    struct VertexBufferComponent {
    std::span<float> vertex;
};

struct IndexBufferComponent {
    std::span<uint16_t> indices;
    u32 indices_size;
    render::IndexFormat format;
};


struct UniformComponent {
    u32 uniform_size;
};
}