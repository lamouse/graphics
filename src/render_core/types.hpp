#pragma once
#include "common/slot_vector.hpp"

namespace render {
using BufferId = common::SlotId;
using VertexAttributeId = common::SlotId;
using VertexBindingsId = common::SlotId;
using ComputeBindingId = common::SlotId;
using MeshId = common::SlotId;
using TextureId = common::SlotId;

struct RenderCommand {
        uint32_t indexOffset{};
        uint32_t indexCount{};
};
}  // namespace render