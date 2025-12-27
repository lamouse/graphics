//
// Created by ziyu on 2025/9/20.
//
#pragma once

#include "common/common_types.hpp"
#include "common/bit_field.hpp"
#include <string>
namespace render {
struct VertexBinding {
        std::uint32_t binding{};
        std::uint32_t stride{};
        bool is_instance{};
        std::uint32_t divisor{1};
};

struct VertexAttribute {
        enum class Size : u32 {
            Invalid = 0x0,
            R32_G32_B32_A32 = 0x01,
            R32_G32_B32 = 0x02,
            R16_G16_B16_A16 = 0x03,
            R32_G32 = 0x04,
            R16_G16_B16 = 0x05,
            R8_G8_B8_A8 = 0x0A,
            R16_G16 = 0x0F,
            R32 = 0x12,
            R8_G8_B8 = 0x13,
            R8_G8 = 0x18,
            R16 = 0x1B,
            R8 = 0x1D,
            A2_B10_G10_R10 = 0x30,
            B10_G11_R11 = 0x31,
            G8_R8 = 0x32,
            X8_B8_G8_R8 = 0x33,
            A8 = 0x34,
        };

        enum class Type : u32 {
            UnusedEnumDoNotUseBecauseItWillGoAway = 0,
            SNorm = 1,
            UNorm = 2,
            SInt = 3,
            UInt = 4,
            UScaled = 5,
            SScaled = 6,
            Float = 7,
        };
        union {
                BitField<0, 1, u32> enable;
                BitField<1, 5, u32> location;
                BitField<6, 14, u32> offset;
                BitField<20, 6, Size> size;
                BitField<28, 3, Type> type;
                u32 hex;
        };

        [[nodiscard]] auto ComponentCount() const -> u32 {
            switch (size) {
                case Size::R32_G32_B32_A32:
                    return 4;
                case Size::R32_G32_B32:
                    return 3;
                case Size::R16_G16_B16_A16:
                    return 4;
                case Size::R32_G32:
                    return 2;
                case Size::R16_G16_B16:
                    return 3;
                case Size::R8_G8_B8_A8:
                case Size::X8_B8_G8_R8:
                    return 4;
                case Size::R16_G16:
                    return 2;
                case Size::R32:
                    return 1;
                case Size::R8_G8_B8:
                    return 3;
                case Size::R8_G8:
                case Size::G8_R8:
                    return 2;
                case Size::R16:
                    return 1;
                case Size::R8:
                case Size::A8:
                    return 1;
                case Size::A2_B10_G10_R10:
                    return 4;
                case Size::B10_G11_R11:
                    return 3;
                default:
                    return 1;
            }
        }

        [[nodiscard]] auto SizeInBytes() const -> u32 {
            switch (size) {
                case Size::R32_G32_B32_A32:
                    return 16;
                case Size::R32_G32_B32:
                    return 12;
                case Size::R16_G16_B16_A16:
                    return 8;
                case Size::R32_G32:
                    return 8;
                case Size::R16_G16_B16:
                    return 6;
                case Size::R8_G8_B8_A8:
                case Size::X8_B8_G8_R8:
                    return 4;
                case Size::R16_G16:
                    return 4;
                case Size::R32:
                    return 4;
                case Size::R8_G8_B8:
                    return 3;
                case Size::R8_G8:
                case Size::G8_R8:
                    return 2;
                case Size::R16:
                    return 2;
                case Size::R8:
                case Size::A8:
                    return 1;
                case Size::A2_B10_G10_R10:
                    return 4;
                case Size::B10_G11_R11:
                    return 4;
                default:
                    return 1;
            }
        }

        [[nodiscard]] auto SizeString() const -> std::string {
            switch (size) {
                case Size::R32_G32_B32_A32:
                    return "32_32_32_32";
                case Size::R32_G32_B32:
                    return "32_32_32";
                case Size::R16_G16_B16_A16:
                    return "16_16_16_16";
                case Size::R32_G32:
                    return "32_32";
                case Size::R16_G16_B16:
                    return "16_16_16";
                case Size::R8_G8_B8_A8:
                    return "8_8_8_8";
                case Size::R16_G16:
                    return "16_16";
                case Size::R32:
                    return "32";
                case Size::R8_G8_B8:
                    return "8_8_8";
                case Size::R8_G8:
                case Size::G8_R8:
                    return "8_8";
                case Size::R16:
                    return "16";
                case Size::R8:
                case Size::A8:
                    return "8";
                case Size::A2_B10_G10_R10:
                    return "2_10_10_10";
                case Size::B10_G11_R11:
                    return "10_11_11";
                default:
                    return {};
            }
        }

        [[nodiscard]] auto TypeString() const -> std::string {
            switch (type) {
                case Type::UnusedEnumDoNotUseBecauseItWillGoAway:
                    return "Unused";
                case Type::SNorm:
                    return "SNORM";
                case Type::UNorm:
                    return "UNORM";
                case Type::SInt:
                    return "SINT";
                case Type::UInt:
                    return "UINT";
                case Type::UScaled:
                    return "USCALED";
                case Type::SScaled:
                    return "SSCALED";
                case Type::Float:
                    return "FLOAT";
            }
            return {};
        }

        auto IsNormalized() const -> bool { return (type == Type::SNorm) || (type == Type::UNorm); }

        auto IsValid() const -> bool { return size != Size::Invalid; }

        auto operator<(const VertexAttribute& other) const -> bool { return hex < other.hex; }
};
static_assert(sizeof(VertexAttribute) == 0x4);
}  // namespace render
