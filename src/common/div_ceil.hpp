// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>

namespace common {

/// Ceiled integer division.
template <typename N, typename D>
    requires std::is_integral_v<N> && std::is_unsigned_v<D>
[[nodiscard]] constexpr auto DivCeil(N number, D divisor) -> N {
    return static_cast<N>((static_cast<D>(number) + divisor - 1) / divisor);
}

/// Ceiled integer division with logarithmic divisor in base 2
template <typename N, typename D>
    requires std::is_integral_v<N> && std::is_unsigned_v<D>
[[nodiscard]] constexpr auto DivCeilLog2(N value, D alignment_log2) -> N {
    return static_cast<N>((static_cast<D>(value) + (D(1) << alignment_log2) - 1) >> alignment_log2);
}

}  // namespace common
