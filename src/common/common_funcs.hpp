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
