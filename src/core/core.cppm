module;
#include <memory>
#include "common/class_traits.hpp"
#include "core/frontend/window.hpp"
#include "render_core/render_base.hpp"
#include "resource/resource.hpp"
export module core;
namespace core {

export class System{
    public:
        System();
        CLASS_NON_COPYABLE(System);
        CLASS_NON_MOVEABLE(System);
        ~System();
        auto Render() -> render::RenderBase&;
        [[nodiscard]] auto Render() const -> render::RenderBase&;
        auto ResourceManager() -> graphics::ResourceManager&;
        auto ResourceManager() const -> graphics::ResourceManager&;

        void run();
        void load(core::frontend::BaseWindow& window);
        void shutdownMainProcess();
        void setShutdown(bool shutdown);
        auto isShutdown() -> bool;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
};
}  // namespace core
