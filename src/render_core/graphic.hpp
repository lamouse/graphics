#pragma once
#include "render_core/fixed_pipeline_state.h"
#include "common/common_funcs.hpp"
#include "render_core/texture/image_info.hpp"
#include <span>
#include <imgui.h>
namespace render {
using GraphicsId = common::SlotId;
struct GraphicsContext {
  texture::ImageInfo image;
  std::span<float> vertex;
  std::span<uint16_t> indices;
  u32 indices_size = 0;
  u32 uniform_size= 0;
  IndexFormat index_format;
};
class Graphic {
    public:
        virtual ~Graphic() = default;
        virtual void setPipelineState(const PipelineState& state) = 0;
        virtual void bindUniformBuffer(GraphicsId id, void* data, size_t size) = 0;
        virtual auto addGraphicContext(const GraphicsContext& context) -> GraphicsId = 0;
        virtual auto getDrawImage() -> ImTextureID = 0;
        virtual void draw(GraphicsId id) = 0;
        virtual void start() = 0;
        virtual void end() = 0;
        Graphic() = default;
        CLASS_DEFAULT_COPYABLE(Graphic);
        CLASS_DEFAULT_MOVEABLE(Graphic);
};
}  // namespace render