//
// Created by ziyu on 2025/9/20.
//
#pragma once
#include <spdlog/spdlog.h>
#include "common/common_funcs.hpp"
#ifndef GRAPHICS_ASSERT_HPP
#define GRAPHICS_ASSERT_HPP
#ifdef _MSC_VER
#define ENGINE_NO_INLINE __declspec(noinline)
#else
#define ENGINE_NO_INLINE __attribute__((noinline))
#endif

#define ASSERT(_a_)                            \
    ([&]() ENGINE_NO_INLINE {                  \
        if (!(_a_)) [[unlikely]] {             \
            SPDLOG_DEBUG("Assertion Failed!"); \
            Crash();                           \
        }                                      \
    }())

#define ASSERT_MSG(_a_, msg)                              \
    ([&]() ENGINE_NO_INLINE {                             \
        if (!(_a_)) [[unlikely]] {                        \
            SPDLOG_DEBUG("Assertion Failed! {} \n", msg); \
            Crash();                                      \
        }                                                 \
    }())

#define UNREACHABLE()                             \
    do {                                          \
        SPDLOG_DEBUG(Debug, "Unreachable code!"); \
        Crash();                                  \
    } while (0)

#define UNREACHABLE_MSG(msg)                         \
    do {                                             \
        SPDLOG_DEBUG("Unreachable code! {} \n" msg); \
        Crash();                                     \
    } while (0)

#ifdef _DEBUG
#define DEBUG_ASSERT(_a_) ASSERT(_a_)
#define DEBUG_ASSERT_MSG(_a_, ...) ASSERT_MSG(_a_, __VA_ARGS__)
#else  // not debug
#define DEBUG_ASSERT(_a_) \
    do {                  \
    } while (0)
#define DEBUG_ASSERT_MSG(_a_, _desc_, ...) \
    do {                                   \
    } while (0)
#endif

#define UNIMPLEMENTED() ASSERT_MSG(false, "Unimplemented code!")
#define UNIMPLEMENTED_MSG(msg) ASSERT_MSG(false, msg)

#define UNIMPLEMENTED_IF(cond) ASSERT_MSG(!(cond), "Unimplemented code!")
#define UNIMPLEMENTED_IF_MSG(cond, ...) ASSERT_MSG(!(cond), __VA_ARGS__)

// If the assert is ignored, execute _b_
#define ASSERT_OR_EXECUTE(_a_, _b_) \
    do {                            \
        ASSERT(_a_);                \
        if (!(_a_)) [[unlikely]] {  \
            _b_                     \
        }                           \
    } while (0)

// If the assert is ignored, execute _b_
#define ASSERT_OR_EXECUTE_MSG(_a_, _b_, ...) \
    do {                                     \
        ASSERT_MSG(_a_, __VA_ARGS__);        \
        if (!(_a_)) [[unlikely]] {           \
            _b_                              \
        }                                    \
    } while (0)
#endif  // GRAPHICS_ASSERT_HPP
