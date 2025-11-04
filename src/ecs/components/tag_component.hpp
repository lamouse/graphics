#pragma once
#include <string>
#include <utility>
#include "common/common_funcs.hpp"
namespace ecs {
struct TagComponent {
        std::string tag;
        TagComponent() = default;
        ~TagComponent() = default;
        CLASS_DEFAULT_COPYABLE(TagComponent);
        CLASS_DEFAULT_MOVEABLE(TagComponent);
        explicit TagComponent(std::string tag) : tag(std::move(tag)) {}
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        operator std::string() const { return tag; }
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        operator const std::string&() const { return tag; }
};

}  // namespace ecs