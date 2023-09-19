#include "g_context.hpp"

#include <cassert>
#include <utility>
#include <vector>
namespace g {
ScreenExtent Context::extent_ = {0, 0};
bool Context::windowIsResize = false;
}  // namespace g