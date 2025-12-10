#pragma once
#include <type_traits>
namespace common {

/**
 * @brief Aligns a given integer value up to the next multiple of the specified size.
 *
 * This function ensures that the provided value is aligned to a boundary specified by the size.
 * It works for any integral type and is constexpr, meaning it can be evaluated at compile time.
 *
 * @tparam T An integral type (e.g., int, unsigned int).
 * @param value_ The value to be aligned.
 * @param size The alignment boundary (must be a power of two).
 * @return T The aligned value, which is the smallest multiple of size that is greater than or equal
 * to value_.
 */
template <typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr auto alignUp(T value_, size_t size) -> T {
    using U = typename std::make_unsigned_t<T>;  // Convert the type to its unsigned equivalent
    auto value{static_cast<U>(value_)};          // Cast value_ to unsigned type
    auto mod{static_cast<T>(value % size)};  // Compute the remainder when value is divided by size
    value -= mod;  // Subtract the remainder to make value a multiple of size
    return static_cast<T>(mod == T{0} ? value
                                      : value + size);  // If mod is not zero, add size to value
}

}  // namespace common