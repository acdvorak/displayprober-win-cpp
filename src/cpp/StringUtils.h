
#pragma once

#include <Windows.h>

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

std::string WideToUtf8(const wchar_t* value);
std::string WideToUtf8(const std::wstring_view& value);

std::wstring Utf8ToWide(std::string_view value);
std::wstring Utf8ToUpperWide(std::string_view value);

std::string ToUpperAscii(std::string_view value);

bool EqualsIgnoreCase(const std::string_view& lhs, const std::string_view& rhs);
bool EqualsIgnoreCase(const std::wstring_view& lhs,
                      const std::wstring_view& rhs);

bool ContainsAnySubstring(std::string_view haystack,
                          std::initializer_list<std::string_view> tokens);

bool HasValue(std::optional<std::string_view> maybe);

std::string Base64Encode(const std::vector<std::uint8_t>& bytes);

std::string BytesToHexUpper(const std::uint8_t* data, std::size_t size);

template <typename T>
inline constexpr bool kHexByteConcatenable =
    std::is_integral_v<T> || std::is_enum_v<T> ||
    std::is_trivially_copyable_v<T>;

template <typename T>
inline void AppendRawBytes(std::vector<std::uint8_t>& out, const T& value) {
  static_assert(kHexByteConcatenable<T>,
                "IntsToHex only accepts integral, enum, or trivially copyable "
                "types");

  if constexpr (std::is_integral_v<T>) {
    using UnsignedT = std::make_unsigned_t<T>;
    UnsignedT normalized = static_cast<UnsignedT>(value);
    for (std::size_t i = sizeof(UnsignedT); i > 0; --i) {
      const std::size_t shift = (i - 1) * 8;
      out.push_back(u8(normalized >> shift));
    }
    return;
  }

  if constexpr (std::is_enum_v<T>) {
    using UnderlyingT = std::underlying_type_t<T>;
    using UnsignedT = std::make_unsigned_t<UnderlyingT>;
    UnsignedT normalized = static_cast<UnsignedT>(value);
    for (std::size_t i = sizeof(UnsignedT); i > 0; --i) {
      const std::size_t shift = (i - 1) * 8;
      out.push_back(u8(normalized >> shift));
    }
    return;
  }

  const auto* begin = reinterpret_cast<const std::uint8_t*>(&value);
  out.insert(out.end(), begin, begin + sizeof(T));
}

template <typename T>
std::string IntsToHex(const T& value) {
  static_assert(kHexByteConcatenable<T>,
                "IntsToHex only accepts integral, enum, or trivially copyable "
                "types");

  std::vector<std::uint8_t> bytes;
  bytes.reserve(sizeof(T));
  AppendRawBytes(bytes, value);

  return BytesToHexUpper(bytes.data(), bytes.size());
}

template <typename... Ts>
std::string IntsToHex(const Ts&... values) {
  static_assert((kHexByteConcatenable<Ts> && ...),
                "IntsToHex only accepts integral, enum, or trivially copyable "
                "types");

  std::vector<std::uint8_t> bytes;
  bytes.reserve((sizeof(Ts) + ... + 0));
  (AppendRawBytes(bytes, values), ...);

  return BytesToHexUpper(bytes.data(), bytes.size());
}

template <typename T>
std::string IntsToHex(const T* data, std::size_t count) {
  static_assert(kHexByteConcatenable<T>,
                "IntsToHex only accepts integral, enum, or trivially copyable "
                "types");

  std::vector<std::uint8_t> bytes;
  bytes.reserve(count * sizeof(T));

  for (std::size_t i = 0; i < count; ++i) {
    AppendRawBytes(bytes, data[i]);
  }

  return BytesToHexUpper(bytes.data(), bytes.size());
}

inline bool StartsWith(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}
