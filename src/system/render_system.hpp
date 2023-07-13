#pragma once
namespace engine::system {

template <typename F>
class RenderSystem {
    public:
        virtual void render(F&) = 0;
};

}  // namespace engine::system
