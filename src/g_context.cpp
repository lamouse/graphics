#include "g_context.hpp"

namespace g {
ScreenExtent Context::extent_ = {0, 0};
bool Context::windowIsResize = false;
}  // namespace g