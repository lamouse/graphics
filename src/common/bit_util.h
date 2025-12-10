// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <bit>
#include <climits>
#include <cstddef>
#include <type_traits>

#include "common_types.hpp"

namespace common {

/// Gets the size of a specified type T in bits.
template <typename T>
[[nodiscard]] constexpr auto BitSize() -> std::size_t {
    return sizeof(T) * CHAR_BIT;
}

[[nodiscard]] constexpr auto MostSignificantBit32(const u32 value) -> u32 {
    return 31U - static_cast<u32>(std::countl_zero(value));
}

[[nodiscard]] constexpr auto MostSignificantBit64(const u64 value) -> u32 {
    return 63U - static_cast<u32>(std::countl_zero(value));
}

[[nodiscard]] constexpr auto Log2Floor32(const u32 value) -> u32 {
    return MostSignificantBit32(value);
}

[[nodiscard]] constexpr auto Log2Floor64(const u64 value) -> u32 {
    return MostSignificantBit64(value);
}

[[nodiscard]] constexpr auto Log2Ceil32(const u32 value) -> u32 {
    const u32 log2_f = Log2Floor32(value);
    return log2_f + static_cast<u32>((value ^ (1U << log2_f)) != 0U);
}

[[nodiscard]] constexpr auto Log2Ceil64(const u64 value) -> u32 {
    const u64 log2_f = Log2Floor64(value);
    return static_cast<u32>(log2_f + static_cast<u64>((value ^ (1ULL << log2_f)) != 0ULL));
}

template <typename T>
    requires std::is_unsigned_v<T>
[[nodiscard]] constexpr auto IsPow2(T value) -> bool {
    return std::has_single_bit(value);
}

template <typename T>
    requires std::is_integral_v<T>
[[nodiscard]] auto NextPow2(T value) -> T {
    return static_cast<T>(1ULL << ((8U * sizeof(T)) - std::countl_zero(value - 1U)));
}

template <size_t bit_index, typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr auto Bit(const T value) -> bool {
    static_assert(bit_index < BitSize<T>(), "bit_index must be smaller than size of T");
    return ((value >> bit_index) & T(1)) == T(1);
}

}  // namespace common
