
#pragma once

#include <Windows.h>

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

std::string WideToUtf8(const wchar_t* value);
std::string WideToUtf8(const std::wstring_view& value);

std::wstring Utf8ToWide(std::string_view value);
std::wstring Utf8ToUpperWide(std::string_view value);

std::string ToUpperAscii(std::string_view value);

std::string Base64Encode(const std::vector<std::uint8_t>& bytes);

std::string BytesToHexUpper(std::span<const std::uint8_t> bytes);

template <typename T>
inline constexpr bool kHexByteConcatenable =
    std::is_integral_v<T> || std::is_enum_v<T> ||
    std::is_trivially_copyable_v<T>;

template <typename T>
inline void AppendRawBytes(std::vector<std::uint8_t>& out, const T& value) {
  static_assert(kHexByteConcatenable<T>,
                "IntsToHex only accepts integral, enum, or trivially copyable "
                "types");

  const auto* begin = reinterpret_cast<const std::uint8_t*>(&value);
  out.insert(out.end(), begin, begin + sizeof(T));
}

template <typename... Ts>
std::string IntsToHex(const Ts&... values) {
  static_assert((kHexByteConcatenable<Ts> && ...),
                "IntsToHex only accepts integral, enum, or trivially copyable "
                "types");

  std::vector<std::uint8_t> bytes;
  bytes.reserve((sizeof(Ts) + ... + 0));
  (AppendRawBytes(bytes, values), ...);

  return BytesToHexUpper(bytes);
}

template <typename T>
std::string IntsToHex(std::span<const T> values) {
  static_assert(kHexByteConcatenable<T>,
                "IntsToHex only accepts integral, enum, or trivially copyable "
                "types");

  std::vector<std::uint8_t> bytes;
  bytes.reserve(values.size() * sizeof(T));

  for (const auto& value : values) {
    AppendRawBytes(bytes, value);
  }

  return BytesToHexUpper(bytes);
}

bool EqualsIgnoreCase(const std::string_view& lhs, const std::string_view& rhs);
bool EqualsIgnoreCase(const std::wstring_view& lhs,
                      const std::wstring_view& rhs);

bool ContainsAnyToken(std::string_view haystack,
                      std::initializer_list<std::string_view> tokens);

std::string Base64Encode(const std::vector<std::uint8_t>& bytes);
