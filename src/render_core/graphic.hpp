#pragma once
#include "render_core/types.hpp"
#include "render_core/shader_cache.hpp"
#include "render_core/compute_instance.hpp"
#include "render_core/texture/types.hpp"
#include "common/common_funcs.hpp"
#include "resource/instance.hpp"
#include "resource/texture/image.hpp"
#include "resource/obj/mesh.hpp"
#include <imgui.h>
#include <ktx.h>
namespace render {
using GraphicsId = common::SlotId;

struct Framebuffer {
        std::array<render::surface::PixelFormat, 8> color_formats{};
        render::surface::PixelFormat depth_format = surface::PixelFormat::Invalid;
        texture::Extent3D extent{};
        Framebuffer() { color_formats.fill(surface::PixelFormat::Invalid); }
};

struct CleanValue {
        Framebuffer framebuffer;
        std::array<float, 4> clear_color{1.f, 1.f, 1.f, 1.f};  // RGBA 0 ~ 1
        float clear_depth{1.f};                                // min max
        std::int32_t offset_x{};
        std::int32_t offset_y{};
        uint32_t width{};
        uint32_t hight{};
};

class Graphic {
    public:
        virtual ~Graphic() = default;
        virtual auto getDrawImage() -> ImTextureID = 0;
        virtual auto uploadModel(const graphics::IMeshData& instance) -> MeshId = 0;
        virtual auto uploadTexture(const ::resource::image::ITexture& texture) -> TextureId = 0;
        virtual auto uploadTexture(ktxTexture* ktxTexture) -> TextureId = 0;
        virtual void draw(const graphics::IMeshInstance& instance) = 0;
        /**
         * @brief 添加shader，返回shader的hash，同过设置IModelInstance设置shader hash
         * 确认使用的shader
         *
         * @param data
         * @param type
         * @return u64
         */
        virtual auto addShader(std::span<const u32> data, ShaderType type) -> u64 = 0;
        virtual void dispatchCompute(const IComputeInstance& instance) = 0;
        virtual void clean(const CleanValue& cleanValue) = 0;
        Graphic() = default;
        CLASS_DEFAULT_COPYABLE(Graphic);
        CLASS_DEFAULT_MOVEABLE(Graphic);
};
}  // namespace render