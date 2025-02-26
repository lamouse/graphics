
#ifndef TRACY_H
#define TRACY_H
#include <tracy/Tracy.hpp>
#if defined(USE_TRACY)
#include <tracy/Tracy.hpp>
#else
#define ZoneScopedN(var)
#endif
#endif //TRACY_H
