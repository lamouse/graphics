module;
#include "common/common_types.hpp"

export module render.vulkan.present:push_constants;

export namespace render::vulkan {

struct ScreenRectVertex {
    ScreenRectVertex() = default;
    explicit ScreenRectVertex(f32 x, f32 y, f32 u, f32 v)
        : position{{x, y}}, tex_coord{{u, v}} {}

    std::array<f32, 2> position;
    std::array<f32, 2> tex_coord;
};

inline auto MakeOrthographicMatrix(f32 width, f32 height) -> std::array<f32, 4 * 4> {
    // clang-format off
    return { 2.f / width, 0.f,          0.f, 0.f,
             0.f,         2.f / height, 0.f, 0.f,
             0.f,         0.f,          1.f, 0.f,
            -1.f,        -1.f,          0.f, 1.f};
    // clang-format on
}

struct PresentPushConstants {
    std::array<f32, 4 * 4> model_view_matrix;
    std::array<ScreenRectVertex, 4> vertices;
};

static_assert(sizeof(PresentPushConstants) <= 128, "Push constants are too large");

}  // namespace render::vulkan