module;
#include <string>
export module render.texture.formatter;
import render.texture.image;
import render.texture.image_view_base;
import render.texture.render_targets;
export namespace render::texture {
[[nodiscard]] auto Name(const ImageInfo& image) -> std::string;

[[nodiscard]] auto Name(const ImageViewBase& image_view) -> std::string;

[[nodiscard]] auto Name(const RenderTargets& render_targets) -> std::string;

}  // namespace render::texture


