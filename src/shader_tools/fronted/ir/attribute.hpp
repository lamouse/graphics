#pragma once
#include <format>
#include <fmt/format.h>
namespace shader::IR {
// clang-format off
    #define ATTRIBUTES(f) \
        f(PrimitiveId, 24)\
        f(Layer, 25)    \
        f(ViewportIndex ,26) \
        f(PointSize, 27) \
        f(PositionX, 28) \
        f(PositionY, 29) \
        f(PositionZ, 30) \
        f(PositionW, 31) \
        f(Generic0X, 32) \
        f(Generic0Y, 33) \
        f(Generic0Z, 34) \
        f(Generic0W, 35) \
        f(Generic1X, 36) \
        f(Generic1Y, 37) \
        f(Generic1Z, 38) \
        f(Generic1W, 39) \
        f(Generic2X, 40) \
        f(Generic2Y, 41) \
        f(Generic2Z, 42) \
        f(Generic2W, 43) \
        f(Generic3X, 44) \
        f(Generic3Y, 45) \
        f(Generic3Z, 46) \
        f(Generic3W, 47) \
        f(Generic4X, 48) \
        f(Generic4Y, 49) \
        f(Generic4Z, 50) \
        f(Generic4W, 51) \
        f(Generic5X, 52) \
        f(Generic5Y, 53) \
        f(Generic5Z, 54) \
        f(Generic5W, 55) \
        f(Generic6X, 56) \
        f(Generic6Y, 57) \
        f(Generic6Z, 58) \
        f(Generic6W, 59) \
        f(Generic7X, 60) \
        f(Generic7Y, 61) \
        f(Generic7Z, 62) \
        f(Generic7W, 63) \
        f(Generic8X, 64) \
        f(Generic8Y, 65) \
        f(Generic8Z, 66) \
        f(Generic8W, 67) \
        f(Generic9X, 68) \
        f(Generic9Y, 69) \
        f(Generic9Z, 70) \
        f(Generic9W, 71) \
        f(Generic10X, 72)   \
        f(Generic10Y, 73)   \
        f(Generic10Z, 74)   \
        f(Generic10W, 75)   \
        f(Generic11X, 76)   \
        f(Generic11Y, 77)   \
        f(Generic11Z, 78)   \
        f(Generic11W, 79)   \
        f(Generic12X, 80)   \
        f(Generic12Y, 81)   \
        f(Generic12Z, 82)   \
        f(Generic12W, 83)   \
        f(Generic13X, 84)   \
        f(Generic13Y, 85)   \
        f(Generic13Z, 86)   \
        f(Generic13W, 87)   \
        f(Generic14X, 88)   \
        f(Generic14Y, 89)   \
        f(Generic14Z, 90)   \
        f(Generic14W, 91)   \
        f(Generic15X, 92)   \
        f(Generic15Y, 93)   \
        f(Generic15Z, 94)   \
        f(Generic15W, 95)   \
        f(Generic16X, 96)   \
        f(Generic16Y, 97)   \
        f(Generic16Z, 98)   \
        f(Generic16W, 99)   \
        f(Generic17X, 100)  \
        f(Generic17Y, 101)  \
        f(Generic17Z, 102)  \
        f(Generic17W, 103)  \
        f(Generic18X, 104)  \
        f(Generic18Y, 105)  \
        f(Generic18Z, 106)  \
        f(Generic18W, 107)  \
        f(Generic19X, 108)  \
        f(Generic19Y, 109)  \
        f(Generic19Z, 110)  \
        f(Generic19W, 111)  \
        f(Generic20X, 112)  \
        f(Generic20Y, 113)  \
        f(Generic20Z, 114)  \
        f(Generic20W, 115)  \
        f(Generic21X, 116)  \
        f(Generic21Y, 117)  \
        f(Generic21Z, 118)  \
        f(Generic21W, 119)  \
        f(Generic22X, 120)  \
        f(Generic22Y, 121)  \
        f(Generic22Z, 122)  \
        f(Generic22W, 123)  \
        f(Generic23X, 124)  \
        f(Generic23Y, 125)  \
        f(Generic23Z, 126)  \
        f(Generic23W, 127)  \
        f(Generic24X, 128)  \
        f(Generic24Y, 129)  \
        f(Generic24Z, 130)  \
        f(Generic24W, 131)  \
        f(Generic25X, 132)  \
        f(Generic25Y, 133)  \
        f(Generic25Z, 134)  \
        f(Generic25W, 135)  \
        f(Generic26X, 136)  \
        f(Generic26Y, 137)  \
        f(Generic26Z, 138)  \
        f(Generic26W, 139)  \
        f(Generic27X, 140)  \
        f(Generic27Y, 141)  \
        f(Generic27Z, 142)  \
        f(Generic27W, 143)  \
        f(Generic28X, 144)  \
        f(Generic28Y, 145)  \
        f(Generic28Z, 146)  \
        f(Generic28W, 147)  \
        f(Generic29X, 148)  \
        f(Generic29Y, 149)  \
        f(Generic29Z, 150)  \
        f(Generic29W, 151)  \
        f(Generic30X, 152)  \
        f(Generic30Y, 153)  \
        f(Generic30Z, 154)  \
        f(Generic30W, 155)  \
        f(Generic31X, 156)  \
        f(Generic31Y, 157)  \
        f(Generic31Z, 158)  \
        f(Generic31W, 159)  \
        f(ColorFrontDiffuseR, 160)   \
        f(ColorFrontDiffuseG, 161)   \
        f(ColorFrontDiffuseB, 162)   \
        f(ColorFrontDiffuseA, 163)   \
        f(ColorFrontSpecularR, 164)   \
        f(ColorFrontSpecularG, 165)   \
        f(ColorFrontSpecularB, 166)   \
        f(ColorFrontSpecularA, 167)   \
        f(ColorBackDiffuseR, 168)   \
        f(ColorBackDiffuseG, 169)   \
        f(ColorBackDiffuseB, 170)   \
        f(ColorBackDiffuseA, 171)   \
        f(ColorBackSpecularR, 172)  \
        f(ColorBackSpecularG, 173)  \
        f(ColorBackSpecularB, 174)  \
        f(ColorBackSpecularA, 175)  \
        f(ClipDistance0, 176)  \
        f(ClipDistance1, 177)  \
        f(ClipDistance2, 178)  \
        f(ClipDistance3, 179)  \
        f(ClipDistance4, 180)  \
        f(ClipDistance5, 181)  \
        f(ClipDistance6, 182)  \
        f(ClipDistance7, 183)  \
        f(PointSpriteS, 184)  \
        f(PointSpriteT, 185)  \
        f(FogCoordinate, 186)  \
        f(TessellationEvaluationPointU, 188)  \
        f(TessellationEvaluationPointV, 189)  \
        f(InstanceId, 190)  \
        f(VertexId, 191)  \
        f(FixedFncTexture0S, 192)  \
        f(FixedFncTexture0T, 193)  \
        f(FixedFncTexture0R, 194)  \
        f(FixedFncTexture0Q, 195)  \
        f(FixedFncTexture1S, 196)  \
        f(FixedFncTexture1T, 197)  \
        f(FixedFncTexture1R, 198)  \
        f(FixedFncTexture1Q, 199)  \
        f(FixedFncTexture2S, 200)  \
        f(FixedFncTexture2T, 201)  \
        f(FixedFncTexture2R, 202)  \
        f(FixedFncTexture2Q, 203)  \
        f(FixedFncTexture3S, 204)  \
        f(FixedFncTexture3T, 205)  \
        f(FixedFncTexture3R, 206)  \
        f(FixedFncTexture3Q, 207)  \
        f(FixedFncTexture4S, 208)  \
        f(FixedFncTexture4T, 209)  \
        f(FixedFncTexture4R, 210)  \
        f(FixedFncTexture4Q, 211)  \
        f(FixedFncTexture5S, 212)  \
        f(FixedFncTexture5T, 213)  \
        f(FixedFncTexture5R, 214)  \
        f(FixedFncTexture5Q, 215)  \
        f(FixedFncTexture6S, 216)  \
        f(FixedFncTexture6T, 217)  \
        f(FixedFncTexture6R, 218)  \
        f(FixedFncTexture6Q, 219)  \
        f(FixedFncTexture7S, 220)  \
        f(FixedFncTexture7T, 221)  \
        f(FixedFncTexture7R, 222)  \
        f(FixedFncTexture7Q, 223)  \
        f(FixedFncTexture8S, 224)  \
        f(FixedFncTexture8T, 225)  \
        f(FixedFncTexture8R, 226)  \
        f(FixedFncTexture8Q, 227)  \
        f(FixedFncTexture9S, 228)  \
        f(FixedFncTexture9T, 229)  \
        f(FixedFncTexture9R, 230)  \
        f(FixedFncTexture9Q, 231)  \
        f(ViewportMask, 232) \
        f(FrontFace, 255) \
        f(BaseInstance, 256) \
        f(BaseVertex, 257) \
        f(DrawID, 258)

// clang-format on

enum class Attribute : uint64_t {
#define FUNC_ATTRIBUTES(name, v) name = (v),
    ATTRIBUTES(FUNC_ATTRIBUTES)
#undef FUNC_ATTRIBUTES
};

constexpr size_t NUM_GENERICS = 32;

constexpr size_t NUM_FIXEDFNCTEXTURE = 10;

[[nodiscard]] auto isGeneric(Attribute attribute) noexcept -> bool;

[[nodiscard]] auto genericAttributeIndex(Attribute attribute) -> uint32_t;

[[nodiscard]] auto genericAttributeElement(Attribute attribute) -> uint32_t;

[[nodiscard]] auto nameOf(Attribute attribute) -> std::string;

[[nodiscard]] constexpr auto operator+(IR::Attribute attribute, size_t value) noexcept -> IR::Attribute {
    return static_cast<IR::Attribute>(static_cast<size_t>(attribute) + value);
}
}  // namespace shader::IR

template <>
struct std::formatter<shader::IR::Attribute> {
        static constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
        template <typename FormatContext>
        auto format(const shader::IR::Attribute& attribute, FormatContext& ctx) const {
            return std::format_to(ctx.out(), "{}", shader::IR::nameOf(attribute));
        }
};

template <>
struct fmt::formatter<shader::IR::Attribute> {
        static constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
        template <typename FormatContext>
        auto format(const shader::IR::Attribute& attribute, FormatContext& ctx) const {
            return fmt::format_to(ctx.out(), "{}", shader::IR::nameOf(attribute));
        }
};
