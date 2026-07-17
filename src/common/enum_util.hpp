#pragma once
#include <meta>
#include <string_view>
#include <optional>
namespace common {

template<typename E, bool B = std::meta::is_enumerable_type(^^E)>
constexpr auto enum_infos()
{
  static_assert(B && "type isn't enum");
  return std::meta::reflect_constant_array(std::meta::enumerators_of(^^E));
}
template<typename E>
  constexpr E enum_first()
{
  constexpr auto infos = enum_infos<E>();
  template for (constexpr std::meta::info I : [:infos:])
  return [:I:];
}

template<typename E>
constexpr E enum_last()
{
  constexpr auto infos = enum_infos<E>();
  auto Enum = enum_first<E>();
  template for (constexpr std::meta::info I : [:infos:])
  {
    Enum = [:I:];
  }
  return Enum;
}

template <typename E, bool B = is_enumerable_type(^^E)>
constexpr auto enum_to_string(E e) -> std::string_view {
    if constexpr (B) {
        constexpr std::meta::info Enums = std::meta::reflect_constant_array(enumerators_of(^^E));
        template for (constexpr std::meta::info I : [:Enums:])
          if (e == [:I:])
            return std::meta::identifier_of(I);
    }
    return "<unnamed>";
}

template <typename E, bool B = is_enumerable_type(^^E)>
constexpr auto string_to_enum(std::string_view s) -> std::optional<E> {
  if constexpr (B) {
    constexpr std::meta::info Enums = std::meta::reflect_constant_array(enumerators_of(^^E));
    template for (constexpr std::meta::info I : [:Enums:])
      if (s == std::meta::identifier_of(I))
        return [:I:];
  }
  return std::nullopt;
}

template <typename E, bool B = is_enumerable_type(^^E)>
constexpr auto enum_string_list() {
  if constexpr (B) {
    constexpr auto Enums = std::meta::reflect_constant_array(enumerators_of(^^E));
    std::vector<std::string_view> names{};
    size_t i = 0;
    template for (constexpr std::meta::info I : [:Enums:]) {
      names.push_back(std::meta::identifier_of(I));
    }
    return names;
  } else {
    return std::vector<std::string_view>{};
  }
}

template <typename E, bool B = is_enumerable_type(^^E)>
constexpr auto enum_to_c_str_list() {
if constexpr (B) {
  constexpr auto Enums = std::meta::reflect_constant_array(enumerators_of(^^E));
  std::vector<const char*> names{};
  size_t i = 0;
  template for (constexpr std::meta::info I : [:Enums:]) {
    names.push_back(std::meta::identifier_of(I).data());
  }
  return names;
} else {
  return std::vector<std::string_view>{};
}
}

}  // namespace common