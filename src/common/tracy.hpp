
#ifndef TRACY_H
#define TRACY_H
#if defined(USE_TRACY)
#define TRACY_ENABLE
#include <tracy/Tracy.hpp>
#else
#define ZoneScopedN(var)
#endif
#endif //TRACY_H
