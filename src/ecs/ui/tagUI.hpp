#pragma once
#include <imgui.h>
#include "ecs/components/tag_component.hpp"
#include <glm/gtc/type_ptr.hpp>
namespace ecs {
inline void DrawCameraUI(TagComponent& tag) {
    // NOLINTNEXTLINE(hicpp-vararg)
    ImGui::Text("%s", tag.tag.c_str());
}
}  // namespace ecs