#include "settings.hpp"
namespace settings {
RenderVulkan RenderVulkan::instance_ = {};

Values values;

#define SETTING(TYPE, RANGED) template class Setting<TYPE, RANGED>
SETTING(enums::VSyncMode, true);
SETTING(enums::LogLevel, true);
SETTING(bool, false);
#undef SETTING

auto TranslateCategory(Category category) -> const char* {
    switch (category) {
        case Category::core:
            return "Core";
        case Category::render:
            return "render";
        case Category::log:
            return "log";
        case Category::system:
            return "system";
        case Category::MAX_EUM:
            break;
    }
    return "Miscellaneous";
}
}