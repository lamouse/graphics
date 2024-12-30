#pragma once
#include <cstdint>
namespace common::literals {

constexpr uint64_t operator""_KiB(unsigned long long int x) { return 1024ULL * x; }

constexpr uint64_t operator""_MiB(unsigned long long int x) { return 1024_KiB * x; }

constexpr uint64_t operator""_GiB(unsigned long long int x) { return 1024_MiB * x; }

constexpr uint64_t operator""_TiB(unsigned long long int x) { return 1024_GiB * x; }

constexpr uint64_t operator""_PiB(unsigned long long int x) { return 1024_TiB * x; }

}  // namespace common::literals