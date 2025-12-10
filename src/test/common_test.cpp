#include <gtest/gtest.h>
#include "common/bit_field.hpp"
#include <format>
#include <print>
TEST(BitFieldTest, CommonBitField) {
    struct MyStruct {
            union {
                    u32 u;
                    BitField<0, 1, u32> bit1;
                    BitField<1, 1, u32> bit2;
                    BitField<2, 1, u32> bit3;
                    BitField<3, 1, u32> bit4;
                    BitField<0, 30, u32> bit32;
            };
    };

    MyStruct s;
    s.u = 0xff00;
    s.bit3.Assign(1);
    auto a = s.bit3;
    std::println("3 {}", bool(a));
    std::println("1 {}", bool(s.bit2));
    std::println("2 {}", bool(s.bit1));
    std::println("4 {}", bool(s.bit4));
    std::println("u {}", static_cast<int>(s.u));
}