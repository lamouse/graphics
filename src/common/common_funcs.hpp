#pragma once

/**
 * @brief Macro to make a class non-copyable
 *
 */
#define CLASS_NON_COPYABLE(cls) \
    cls(const cls&) = delete;   \
    auto operator=(const cls&)->cls& = delete
/**
 * @brief Macro to make a class non-moveable
 *
 */
#define CLASS_NON_MOVEABLE(cls) \
    cls(cls&&) = delete;        \
    auto operator=(cls&&)->cls& = delete

#define __FILENAME__ (::std::strrchr(__FILE__, '/') ? ::std::strrchr(__FILE__, '/') + 1 : __FILE__)

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

/**
 * @brief get the current file name, line number and function name
 *
 */
#define SOURCE_CODE_DETAIL_INFO(msg)                                                                           \
    ::std::string("(") + __FILENAME__ + ::std::string(":") + ::std::to_string(__LINE__) + ::std::string(" ") + \
        __PRETTY_FUNCTION__ + ::std::string("): ") + msg

namespace common {}