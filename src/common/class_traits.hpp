#pragma once

/**
 * @brief Macro to make a class non-moveable
 *
 */
#define CLASS_NON_MOVEABLE(cls) \
    cls(cls&&) = delete;        \
    auto operator=(cls&&)->cls& = delete

#define CLASS_DEFAULT_MOVEABLE(cls) \
    cls(cls&&) noexcept = default;  \
    auto operator=(cls&&) noexcept -> cls& = default

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
