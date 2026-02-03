#pragma once
#include "render_core/types.hpp"
#include "render_core/shader_cache.hpp"
#include "render_core/compute_instance.hpp"
#include "render_core/texture/types.hpp"
#include "render_core/render_command.hpp"
#include "render_core/mesh.hpp"
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
        virtual auto getDrawImage() -> unsigned long long = 0;
        virtual auto uploadModel(const IMeshData& instance) -> MeshId = 0;
        virtual auto uploadTexture(const ITexture& texture) -> TextureId = 0;
        virtual auto uploadTexture(ktxTexture* ktxTexture) -> TextureId = 0;
        virtual void draw(const IMeshInstance& instance) = 0;
        virtual void draw(const DrawIndexCommand& command) = 0;

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
        Graphic(const Graphic&) = default;
        Graphic(Graphic&&) noexcept = default;
        auto operator=(const Graphic&) -> Graphic& = default;
        auto operator=(Graphic&&) noexcept ->Graphic& = default;
};
}  // namespace render