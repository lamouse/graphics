module;
#include "render_core/types.hpp"
export module render.types;

#include "common/slot_vector.hpp"

export namespace render {
using BufferId = common::SlotId;
using VertexAttributeId = common::SlotId;
using VertexBindingsId = common::SlotId;
using ComputeBindingId = common::SlotId;
using MeshId = common::SlotId;
using TextureId = common::SlotId;

using RenderCommand = render::RenderCommand;
}  //