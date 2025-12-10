#pragma once
namespace ecs {
struct RenderStateComponent {
        bool visible{true};
        bool mouse_select{};
        unsigned int select_id{};
        unsigned int id{};
        explicit RenderStateComponent(int id_) : id(id_) {}
        [[nodiscard]] auto is_select() const -> bool { return mouse_select || id == select_id; }
};
}  // namespace ecs