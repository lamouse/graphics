// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace settings::enums {

template <typename T>
struct EnumMetadata {
        static std::vector<std::pair<std::string, T>> canonicalizations();
        static uint32_t Index();
        static constexpr T GetFirst();
        static constexpr T GetLast();
};

#define PAIR_45(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_46(N, __VA_ARGS__))
#define PAIR_44(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_45(N, __VA_ARGS__))
#define PAIR_43(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_44(N, __VA_ARGS__))
#define PAIR_42(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_43(N, __VA_ARGS__))
#define PAIR_41(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_42(N, __VA_ARGS__))
#define PAIR_40(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_41(N, __VA_ARGS__))
#define PAIR_39(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_40(N, __VA_ARGS__))
#define PAIR_38(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_39(N, __VA_ARGS__))
#define PAIR_37(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_38(N, __VA_ARGS__))
#define PAIR_36(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_37(N, __VA_ARGS__))
#define PAIR_35(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_36(N, __VA_ARGS__))
#define PAIR_34(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_35(N, __VA_ARGS__))
#define PAIR_33(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_34(N, __VA_ARGS__))
#define PAIR_32(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_33(N, __VA_ARGS__))
#define PAIR_31(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_32(N, __VA_ARGS__))
#define PAIR_30(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_31(N, __VA_ARGS__))
#define PAIR_29(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_30(N, __VA_ARGS__))
#define PAIR_28(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_29(N, __VA_ARGS__))
#define PAIR_27(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_28(N, __VA_ARGS__))
#define PAIR_26(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_27(N, __VA_ARGS__))
#define PAIR_25(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_26(N, __VA_ARGS__))
#define PAIR_24(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_25(N, __VA_ARGS__))
#define PAIR_23(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_24(N, __VA_ARGS__))
#define PAIR_22(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_23(N, __VA_ARGS__))
#define PAIR_21(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_22(N, __VA_ARGS__))
#define PAIR_20(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_21(N, __VA_ARGS__))
#define PAIR_19(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_20(N, __VA_ARGS__))
#define PAIR_18(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_19(N, __VA_ARGS__))
#define PAIR_17(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_18(N, __VA_ARGS__))
#define PAIR_16(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_17(N, __VA_ARGS__))
#define PAIR_15(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_16(N, __VA_ARGS__))
#define PAIR_14(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_15(N, __VA_ARGS__))
#define PAIR_13(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_14(N, __VA_ARGS__))
#define PAIR_12(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_13(N, __VA_ARGS__))
#define PAIR_11(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_12(N, __VA_ARGS__))
#define PAIR_10(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_11(N, __VA_ARGS__))
#define PAIR_9(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_10(N, __VA_ARGS__))
#define PAIR_8(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_9(N, __VA_ARGS__))
#define PAIR_7(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_8(N, __VA_ARGS__))
#define PAIR_6(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_7(N, __VA_ARGS__))
#define PAIR_5(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_6(N, __VA_ARGS__))
#define PAIR_4(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_5(N, __VA_ARGS__))
#define PAIR_3(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_4(N, __VA_ARGS__))
#define PAIR_2(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_3(N, __VA_ARGS__))
#define PAIR_1(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_2(N, __VA_ARGS__))
#define PAIR(N, X, ...) {#X, N::X} __VA_OPT__(, PAIR_1(N, __VA_ARGS__))

#define PP_HEAD(A, ...) A

#define ENUM(NAME, ...)                                                                        \
    enum class NAME : uint32_t { __VA_ARGS__ };                                                \
    template <>                                                                                \
    inline std::vector<std::pair<std::string, NAME>> EnumMetadata<NAME>::canonicalizations() { \
        return {PAIR(NAME, __VA_ARGS__)};                                                      \
    }                                                                                          \
    template <>                                                                                \
    inline uint32_t EnumMetadata<NAME>::Index() {                                              \
        return __COUNTER__;                                                                    \
    }                                                                                          \
    template <>                                                                                \
    inline constexpr NAME EnumMetadata<NAME>::GetFirst() {                                     \
        return NAME::PP_HEAD(__VA_ARGS__);                                                     \
    }                                                                                          \
    template <>                                                                                \
    inline constexpr NAME EnumMetadata<NAME>::GetLast() {                                      \
        return (std::vector<std::pair<std::string_view, NAME>>{PAIR(NAME, __VA_ARGS__)})       \
            .back()                                                                            \
            .second;                                                                           \
    }

ENUM(VSyncMode, Immediate, Mailbox, Fifo, FifoRelaxed);
ENUM(AnisotropyMode, Automatic, Default, X2, X4, X8, X16);
ENUM(AstcDecodeMode, Cpu, Gpu, CpuAsynchronous);
ENUM(AstcRecompression, Uncompressed, Bc1, Bc3);
ENUM(ScalingFilter, NearestNeighbor, Bilinear, Bicubic, Gaussian, ScaleForce, Fsr, MaxEnum);
ENUM(VramUsageMode, Conservative, Aggressive);
ENUM(ShaderBackend, Glsl, Glasm, SpirV);
ENUM(AspectRatio, R16_9, R4_3, R21_9, R16_10, R32_9, Stretch);
ENUM(LogLevel, debug, info, warn, trace, error, critical, off);

template <typename Type>
inline auto CanonicalizeEnum(Type id) -> std::string {
    const auto group = EnumMetadata<Type>::canonicalizations();
    for (auto& [name, value] : group) {
        if (value == id) {
            return name;
        }
    }
    return "unknown";
}

template <typename Type>
inline auto ToEnum(const std::string& canonicalization) -> Type {
    const auto group = EnumMetadata<Type>::canonicalizations();
    for (auto& [name, value] : group) {
        if (name == canonicalization) {
            return value;
        }
    }
    return {};
}
}  // namespace settings::enums

#undef ENUM
#undef PAIR
#undef PAIR_1
#undef PAIR_2
#undef PAIR_3
#undef PAIR_4
#undef PAIR_5
#undef PAIR_6
#undef PAIR_7
#undef PAIR_8
#undef PAIR_9
#undef PAIR_10
#undef PAIR_12
#undef PAIR_13
#undef PAIR_14
#undef PAIR_15
#undef PAIR_16
#undef PAIR_17
#undef PAIR_18
#undef PAIR_19
#undef PAIR_20
#undef PAIR_22
#undef PAIR_23
#undef PAIR_24
#undef PAIR_25
#undef PAIR_26
#undef PAIR_27
#undef PAIR_28
#undef PAIR_29
#undef PAIR_30
#undef PAIR_32
#undef PAIR_33
#undef PAIR_34
#undef PAIR_35
#undef PAIR_36
#undef PAIR_37
#undef PAIR_38
#undef PAIR_39
#undef PAIR_40
#undef PAIR_42
#undef PAIR_43
#undef PAIR_44
#undef PAIR_45
