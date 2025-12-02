#include "settings.hpp"
namespace settings {
Values values;//NOLINT

#define SETTING(TYPE, RANGED) template class Setting<TYPE, RANGED>
SETTING(enums::VSyncMode, true);
SETTING(enums::LogLevel, true);
SETTING(bool, false);
SETTING(int, true);
#undef SETTING

auto TranslateCategory(Category category) -> const char* {
    switch (category) {
        case Category::core:
            return "Core";
        case Category::render:
            return "Render";
        case Category::log:
            return "Log";
        case Category::system:
            return "System";
        case Category::MAX_EUM:
            break;
    }
    return "Miscellaneous";
}
}  // namespace settings