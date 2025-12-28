module;
#include "common/settings.hpp"
#include "common/settings_enums.hpp"
export module common.settings;
export namespace settings{
using Values = settings::Values;
using MenuData = settings::MenuData;
using settings::values;
namespace enums{
using VSyncMode = settings::enums::VSyncMode;
using ScalingFilter = settings::enums::ScalingFilter;
}
}
