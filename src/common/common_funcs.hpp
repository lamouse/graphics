#pragma once
#include <iterator>
#if !defined(ARCHITECTURE_x86_64)
#include <cstdlib>  // for exit
#endif
/**
 * @brief Macro to make a class non-copyable
 *
 */
#define CLASS_NON_COPYABLE(cls) \
    cls(const cls&) = delete;   \
    auto operator=(const cls&)->cls& = delete

#define CLASS_DEFAULT_COPYABLE(cls) \
    cls(const cls&) = default;      \
    auto operator=(const cls&)->cls& = default
/**
 * @brief Macro to make a class non-moveable
 *
 */
#ifndef _MSC_VER
#if defined(ARCHITECTURE_x86_64)
#define Crash() __asm__ __volatile__("int $3")
#elif defined(ARCHITECTURE_arm64)
#define Crash() __asm__ __volatile__("brk #0")
#else
#define Crash() exit(1)
#endif
#else

// Locale Cross-Compatibility
#define locale_t _locale_t
extern "C" {
__declspec(dllimport) void __stdcall DebugBreak(void);
}
#define Crash() DebugBreak()

#endif  // _MSC_VER ndef
#define CLASS_NON_MOVEABLE(cls) \
    cls(cls&&) = delete;        \
    auto operator=(cls&&)->cls& = delete

#define CLASS_DEFAULT_MOVEABLE(cls) \
    cls(cls&&) noexcept = default;  \
    auto operator=(cls&&) noexcept -> cls& = default

#define __FILENAME__ (::std::strrchr(__FILE__, '/') ? ::std::strrchr(__FILE__, '/') + 1 : __FILE__)

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

/**
 * @brief get the current file name, line number and function name
 *
 */
#define SOURCE_CODE_DETAIL_INFO(msg)                                                      \
    ::std::string("(") + __FILENAME__ + ::std::string(":") + ::std::to_string(__LINE__) + \
        ::std::string(" ") + __PRETTY_FUNCTION__ + ::std::string("): ") + msg

#define DECLARE_ENUM_FLAG_OPERATORS(type)                                 \
    [[nodiscard]] constexpr type operator|(type a, type b) noexcept {     \
        using T = std::underlying_type_t<type>;                           \
        return static_cast<type>(static_cast<T>(a) | static_cast<T>(b));  \
    }                                                                     \
    [[nodiscard]] constexpr type operator&(type a, type b) noexcept {     \
        using T = std::underlying_type_t<type>;                           \
        return static_cast<type>(static_cast<T>(a) & static_cast<T>(b));  \
    }                                                                     \
    [[nodiscard]] constexpr type operator^(type a, type b) noexcept {     \
        using T = std::underlying_type_t<type>;                           \
        return static_cast<type>(static_cast<T>(a) ^ static_cast<T>(b));  \
    }                                                                     \
    [[nodiscard]] constexpr type operator<<(type a, type b) noexcept {    \
        using T = std::underlying_type_t<type>;                           \
        return static_cast<type>(static_cast<T>(a) << static_cast<T>(b)); \
    }                                                                     \
    [[nodiscard]] constexpr type operator>>(type a, type b) noexcept {    \
        using T = std::underlying_type_t<type>;                           \
        return static_cast<type>(static_cast<T>(a) >> static_cast<T>(b)); \
    }                                                                     \
    constexpr type& operator|=(type& a, type b) noexcept {                \
        a = a | b;                                                        \
        return a;                                                         \
    }                                                                     \
    constexpr type& operator&=(type& a, type b) noexcept {                \
        a = a & b;                                                        \
        return a;                                                         \
    }                                                                     \
    constexpr type& operator^=(type& a, type b) noexcept {                \
        a = a ^ b;                                                        \
        return a;                                                         \
    }                                                                     \
    constexpr type& operator<<=(type& a, type b) noexcept {               \
        a = a << b;                                                       \
        return a;                                                         \
    }                                                                     \
    constexpr type& operator>>=(type& a, type b) noexcept {               \
        a = a >> b;                                                       \
        return a;                                                         \
    }                                                                     \
    [[nodiscard]] constexpr type operator~(type key) noexcept {           \
        using T = std::underlying_type_t<type>;                           \
        return static_cast<type>(~static_cast<T>(key));                   \
    }                                                                     \
    [[nodiscard]] constexpr bool True(type key) noexcept {                \
        using T = std::underlying_type_t<type>;                           \
        return static_cast<T>(key) != 0;                                  \
    }                                                                     \
    [[nodiscard]] constexpr bool False(type key) noexcept {               \
        using T = std::underlying_type_t<type>;                           \
        return static_cast<T>(key) == 0;                                  \
    }

template <class C>
constexpr auto constexpr_size(const C& c) -> std::size_t {
    if constexpr (sizeof(C) == 0) {
        return 0;
    } else {
        return std::size(c);
    }
}

#define AS_BYTE_SPAN                                                               \
    [[nodiscard]] auto as_byte_span() const -> std::span<const std::byte> {        \
        return std::span{reinterpret_cast<const std::byte*>(this), sizeof(*this)}; \
    }