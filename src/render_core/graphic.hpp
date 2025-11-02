#pragma once
#include "render_core/fixed_pipeline_state.h"
#include "render_core/types.hpp"
#include "render_core/shader_cache.hpp"
#include "render_core/compute_instance.hpp"
#include "common/common_funcs.hpp"
#include "resource/instance.hpp"
#include "resource/texture/image.hpp"
#include "resource/obj/mesh.hpp"
#include <imgui.h>
namespace render {
using GraphicsId = common::SlotId;
class Graphic {
    public:
        virtual ~Graphic() = default;
        virtual void setPipelineState(const DynamicPipelineState& state) = 0;
        virtual auto getDrawImage() -> ImTextureID = 0;
        virtual auto uploadModel(const graphics::IMeshData& instance) -> MeshId = 0;
        virtual auto uploadTexture(const ::resource::image::ITexture& texture ) ->TextureId = 0;
        virtual void draw(const graphics::IModelInstance& instance) = 0;
        /**
         * @brief 添加shader，返回shader的hash，同过设置IModelInstance设置shader hash 确认使用的shader
         *
         * @param data
         * @param type
         * @return u64
         */
        virtual auto addShader(std::span<const u32> data, ShaderType type) -> u64 = 0;
        virtual void dispatchCompute(const IComputeInstance& instance) = 0;
        virtual void clean() = 0;
        Graphic() = default;
        CLASS_DEFAULT_COPYABLE(Graphic);
        CLASS_DEFAULT_MOVEABLE(Graphic);
};
}  // namespace render