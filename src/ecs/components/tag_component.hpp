#pragma once
#include <string>
#include <utility>
namespace ecs {
struct TagComponent {
        std::string tag;
        explicit TagComponent(std::string tag) : tag(std::move(tag)) {}
};

}  // namespace ecs