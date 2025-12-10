#pragma once
#include <cstdint>
namespace common::literals {

constexpr auto operator""_KiB(unsigned long long int x) -> uint64_t { return 1024ULL * x; }

constexpr auto operator""_MiB(unsigned long long int x) -> uint64_t { return 1024_KiB * x; }

constexpr auto operator""_GiB(unsigned long long int x) -> uint64_t { return 1024_MiB * x; }

constexpr auto operator""_TiB(unsigned long long int x) -> uint64_t { return 1024_GiB * x; }

constexpr auto operator""_PiB(unsigned long long int x) -> uint64_t { return 1024_TiB * x; }

}  // namespace common::literals