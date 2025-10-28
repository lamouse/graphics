#pragma once
namespace ecs {
    struct RenderStateComponent{
        bool visible{true};
        unsigned int select_id{};
        unsigned int id{};
        explicit RenderStateComponent(int id_):id(id_){}
        [[nodiscard]] auto is_select() const -> bool{return  id == select_id;}
    };
}