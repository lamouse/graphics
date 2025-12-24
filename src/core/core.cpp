// module;

#include <atomic>
#include "core/core.hpp"

#include "render_core/render_core.hpp"
#include "resource/resource.hpp"
// module core;

namespace core {
struct System::Impl {
    private:
        std::atomic<bool> is_shut_down_;
        std::unique_ptr<render::RenderBase> render_base;
        std::unique_ptr<graphics::ResourceManager> resource_manager;

    public:
        auto Render() -> render::RenderBase* { return render_base.get(); }
        auto isSisShutdown() -> bool { return is_shut_down_.load(); }
        void set_shutdown(bool shutdown) { is_shut_down_ = shutdown; }
        void shutdown_main_process() { render_base.reset(); }
        auto ResourceManager() -> graphics::ResourceManager* { return resource_manager.get(); }
        void load(frontend::BaseWindow& window) {
            render_base = render::createRender(&window);
            resource_manager =
                std::make_unique<graphics::ResourceManager>(render_base->getGraphics());
        }
};
System::System() : impl_(std::make_unique<System::Impl>()) {}
System::~System() = default;
void System::load(core::frontend::BaseWindow& window) { impl_->load(window); }
auto System::Render() -> render::RenderBase& { return *impl_->Render(); }
[[nodiscard]] auto System::Render() const -> render::RenderBase& { return *impl_->Render(); }
void System::setShutdown(bool shutdown) { impl_->set_shutdown(shutdown); }
auto System::isShutdown() -> bool { return impl_->isSisShutdown(); }
void System::shutdownMainProcess() {}
auto System::ResourceManager() -> graphics::ResourceManager& { return *impl_->ResourceManager(); }
auto System::ResourceManager() const -> graphics::ResourceManager& {return *impl_->ResourceManager();}

void System::run() {}
}  // namespace core