#pragma once
#include "core/frontend/window.hpp"
#include "resource/resource.hpp"
#include <memory>

namespace render {
    class RenderBase;
}
namespace graphics::input {
class InputSystem;
}
namespace core {
class System {
    public:
        System();
        ~System();
        System(const System&) = delete;
        System(System&&) noexcept = default;
        auto operator=(const System&) = delete;
        auto operator=(System&&) noexcept -> System& = default;
        auto Render() -> render::RenderBase&;
        [[nodiscard]] auto Render() const -> render::RenderBase&;
        auto ResourceManager() -> graphics::ResourceManager&;
        [[nodiscard]] auto ResourceManager() const -> graphics::ResourceManager&;

        void run(std::shared_ptr<graphics::input::InputSystem> inputSystem);
        void load(core::frontend::BaseWindow& window);
        void shutdownMainProcess();
        void setShutdown(bool shutdown);
        auto isShutdown() -> bool;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
};
}  // namespace core