//
// Created by ziyu on 2025/9/29.
//

#pragma once
#include "ecs/scene/scene.hpp"
#include "common/assert.hpp"

namespace ecs {

class Entity {
        friend class Scene;

    public:
        Entity() = default;
        ~Entity() = default;
        Entity(const Entity&) = default;
        Entity(Entity&&) = default;
        auto operator=(const Entity&) -> Entity& = default;
        auto operator=(Entity&&)noexcept ->Entity& = default;
        template <typename T>
        [[nodiscard]] auto hasComponent() const -> bool {
            return scene_->registry_.all_of<T>(handle_);
        }
        template <typename T, typename... Args>
        auto addComponent(Args&&... args) -> T& {
            // NOLINTNEXTLINE(modernize-use-trailing-return-type)
            ASSERT_MSG(!hasComponent<T>(), "Entity already has component");
            return scene_->registry_.emplace<T>(handle_, std::forward<Args>(args)...);
        }

        template <typename T>
        auto getComponent() -> T& {
            // NOLINTNEXTLINE(modernize-use-trailing-return-type)
            ASSERT_MSG(hasComponent<T>(), "Entity dose not has component");
            return scene_->registry_.get<T>(handle_);
        }

        template <typename T>
        auto getComponent() const -> T& {
            // NOLINTNEXTLINE(modernize-use-trailing-return-type)
            ASSERT_MSG(hasComponent<T>(), "Entity dose not has component");
            return scene_->registry_.get<T>(handle_);
        }

        template <typename T>
        void removeComponent() {
            // NOLINTNEXTLINE(modernize-use-trailing-return-type)
            ASSERT_MSG(!hasComponent<T>(), "Entity dose not has component");
            scene_->registry_.remove<T>(handle_);
        }
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        operator bool() const { return handle_ != entt::null; }
        auto operator==(const Entity& rhs) const noexcept -> bool {
            return this->scene_ == rhs.scene_ && this->handle_ == rhs.handle_;
        }

        auto operator!=(const Entity& rhs) const noexcept -> bool { return !*this == rhs; }

    private:
        Entity(entt::entity handle, Scene* scene) : handle_(handle), scene_(scene) {};
        entt::entity handle_{entt::null};
        Scene* scene_{nullptr};
};

}  // namespace ecs
