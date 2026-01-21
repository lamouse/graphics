module;
#include "common/settings.hpp"
#include "common/settings_enums.hpp"
export module common.settings;
export namespace settings{
using settings::Values;
using settings::MenuData;
using settings::values;
namespace enums{
using settings::enums::VSyncMode;
using settings::enums::ScalingFilter;
}
}
