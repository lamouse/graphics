#pragma once
#include "resource/id.hpp"
#include <vector>
#include <functional>
#include <utility>
#include <unordered_map>
#include "ecs/component.hpp"

// 前向声明
namespace core {
struct FrameInfo;

}
namespace render {
class Graphic;
}

namespace world {
class World;

// 概念约束
template <typename T>
concept DrawableLike =
    requires(T t, const core::FrameInfo& info, world::World& world, render::Graphic* gfx) {
        { t->update(info, world) } -> std::same_as<void>;
        { t->draw(gfx) } -> std::same_as<void>;
        { t->getId() } -> std::same_as<id_t>;
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
                std::function<id_t()> getId;
                std::function<ecs::Entity&()> getEntity;
                std::function<std::vector<ecs::Entity>()> getChildren;

                // 构造函数：接受任何满足 DrawableLike 的类型
                template <DrawableLike T>
                explicit Drawable(T&& obj)
                    : update([o = std::forward<T>(obj)](const core::FrameInfo& info,
                                                        world::World& world) mutable -> auto {
                          o->update(info, world);
                      }),
                      draw([o = std::forward<T>(obj)](render::Graphic* gfx) mutable -> auto {
                          o->draw(gfx);
                      }),
                      getId([o = obj]() { return o->getId(); }),
                      getEntity([o = obj]() -> ecs::Entity& { return o->entity_; }),
                      getChildren([o = obj]() { return o->getChildEntitys(); }) {}

                // 如果需要支持拷贝/移动，确保 std::function 的行为符合预期
                Drawable(const Drawable&) = default;
                Drawable(Drawable&&) = default;
                ~Drawable() = default;
                auto operator=(const Drawable&) -> Drawable& = default;
                auto operator=(Drawable&&) -> Drawable& = default;
        };

        std::vector<Drawable> objects_;
        std::unordered_map<id_t, size_t> id_to_index_;

    public:
        RenderRegistry() = default;

        template <DrawableLike T>
        void add(T obj) {
            objects_.push_back(Drawable{obj});

            using U = typename std::decay_t<T>::element_type;
            id_to_index_[obj->getId()] = objects_.size() - 1;
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

        auto getDrawableById(id_t id) -> Drawable* {
            auto it = id_to_index_.find(id);
            if (it != id_to_index_.end()) {
                return &objects_[it->second];
            }
            return nullptr;
        }

        void processOutlineres(const std::function<void(ecs::Outliner&&)>& func) {
            for (auto& obj : objects_) {
                ecs::Outliner outliner;
                outliner.entity = obj.getEntity();
                outliner.children = obj.getChildren();
                func(std::move(outliner));
            }
        }

        // 控制
        void clear() { objects_.clear(); }
        void reserve(size_t n) { objects_.reserve(n); }
        auto getOutliner(this auto&& self) -> decltype(auto) { return (self.outliner_entities_); };
        [[nodiscard]] auto size() const -> size_t { return objects_.size(); }
        [[nodiscard]] auto empty() const -> bool { return objects_.empty(); }
};
}  // namespace world