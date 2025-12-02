#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <utility>
#include "ecs/scene/entity.hpp"
#include "gui.hpp"
#include "world/world.hpp"

// 前向声明
namespace core {
struct FrameInfo;

}
namespace render {
class Graphic;
}

namespace graphics {
// 概念约束
template <typename T>
concept DrawableLike =
    requires(T t, const core::FrameInfo& info, world::World& world, render::Graphic* gfx) {
        { t->update(info, world) } -> std::same_as<void>;
        { t->draw(gfx) } -> std::same_as<void>;
    };

template <typename T>
concept HasPointECSInterface = requires(T* t) {
    { t->entity_ } -> std::same_as<ecs::Entity&>;
    { t->getChildEntitys() } -> std::same_as<std::vector<ecs::Entity>>;
};

template <typename T>
concept HasValueECSInterface = requires(T t) {
    { t.entity_ } -> std::same_as<ecs::Entity&>;
    { t.getChildEntitys() } -> std::same_as<std::vector<ecs::Entity>>;
};

class RenderRegistry {
    private:
        struct Drawable {
                std::function<void(const core::FrameInfo&, world::World&)> update;
                std::function<void(render::Graphic*)> draw;

                // 构造函数：接受任何满足 DrawableLike 的类型
                template <DrawableLike T>
                explicit Drawable(T&& obj)
                    : update([o = std::forward<T>(obj)](const core::FrameInfo& info,
                                                        world::World& world) mutable -> auto {
                          o->update(info, world);
                      }),
                      draw([o = std::forward<T>(obj)](render::Graphic* gfx) mutable -> auto {
                          o->draw(gfx);
                      }) {}

                // 如果需要支持拷贝/移动，确保 std::function 的行为符合预期
                Drawable(const Drawable&) = default;
                Drawable(Drawable&&) = default;
                ~Drawable() = default;
                auto operator=(const Drawable&) -> Drawable& = default;
                auto operator=(Drawable&&) -> Drawable& = default;
        };

        std::vector<Drawable> objects_;
        std::vector<ui::Outliner> outliner_entities_;

    public:
        RenderRegistry() = default;

        template <typename T>
            requires DrawableLike<T>
        void add(T obj) {
            objects_.push_back(Drawable{obj});
            ui::Outliner outliner;
            // 处理 shared_ptr 和 值类型
            if constexpr (std::is_same_v<std::decay_t<T>,
                                         std::shared_ptr<typename std::decay_t<T>::element_type>>) {
                // T 是 shared_ptr<U>
                using U = typename std::decay_t<T>::element_type;
                if constexpr (HasPointECSInterface<std::decay_t<U>>) {
                    outliner.entity = obj->entity_;
                    outliner.children = obj->getChildEntitys();
                    outliner_entities_.push_back(outliner);
                }
            } else {
                // T 是普通类型（值）
                objects_.push_back(Drawable{obj});
                if constexpr (HasValueECSInterface<std::decay_t<T>>) {
                    outliner.entity = obj->entity_;
                    outliner.children = obj->getChildEntitys();
                    outliner_entities_.push_back(outliner);
                }
            }
        }
        // 批量添加
        template <DrawableLike T, typename... Args>
        void add(T obj, Args... args) {
            add(std::forward<T>(obj));
            if constexpr (sizeof...(Args) > 0) {
                add(std::forward<Args>(args)...);
            }
        }

        // 统一更新和绘制
        void updateAll(const core::FrameInfo& info, world::World& world) {
            for (auto& obj : objects_) {
                obj.update(info, world);
            }
        }

        void drawAll(render::Graphic* gfx) {
            for (auto& obj : objects_) {
                obj.draw(gfx);
            }
        }

        // 控制
        void clear() { objects_.clear(); }
        void reserve(size_t n) { objects_.reserve(n); }
        auto getOutliner(this auto&& self) -> decltype(auto) { return (self.outliner_entities_); };
        [[nodiscard]] auto size() const -> size_t { return objects_.size(); }
        [[nodiscard]] auto empty() const -> bool { return objects_.empty(); }
};
}  // namespace graphics