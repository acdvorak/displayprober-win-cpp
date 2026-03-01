#include "StringUtils.h"

// This header needs to be imported first.
#include <Windows.h>

#include <cctype>
#include <cstdint>
#include <cwctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

std::string WideToUtf8(const wchar_t* value) {
  if (!value || !*value) {
    return {};
  }

  const int required =
      WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
  if (required <= 1) {
    return {};
  }

  std::string utf8(static_cast<size_t>(required), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value, -1, utf8.data(), required, nullptr,
                      nullptr);
  utf8.resize(static_cast<size_t>(required - 1));
  return utf8;
}

std::string WideToUtf8(const std::wstring_view& value) {
  if (value.empty()) {
    return {};
  }

  const int required = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                           static_cast<int>(value.size()),
                                           nullptr, 0, nullptr, nullptr);
  if (required <= 0) {
    return {};
  }

  std::string utf8(static_cast<size_t>(required), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                      utf8.data(), required, nullptr, nullptr);
  return utf8;
}

std::wstring Utf8ToWide(std::string_view value) {
  if (value.empty()) {
    return {};
  }

  const int required = MultiByteToWideChar(
      CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
  if (required <= 0) {
    return {};
  }

  std::wstring wide(static_cast<size_t>(required), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                      wide.data(), required);
  return wide;
}

std::wstring Utf8ToUpperWide(std::string_view value) {
  std::wstring wide = Utf8ToWide(value);
  for (wchar_t& ch : wide) {
    ch = static_cast<wchar_t>(std::towupper(static_cast<wint_t>(ch)));
  }
  return wide;
}

std::string ToUpperAscii(std::string_view value) {
  std::string result(value);
  for (char& ch : result) {
    if (ch >= 'a' && ch <= 'z') {
      ch = static_cast<char>(ch - ('a' - 'A'));
    }
  }
  return result;
}

bool EqualsIgnoreCase(const std::string_view& lhs,
                      const std::string_view& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  bool seenNonAscii = false;
  for (size_t i = 0; i < lhs.size(); ++i) {
    unsigned char left = static_cast<unsigned char>(lhs[i]);
    unsigned char right = static_cast<unsigned char>(rhs[i]);

    if (((left | right) & 0x80u) != 0) {
      seenNonAscii = true;
      break;
    }

    if (left >= 'A' && left <= 'Z') {
      left = static_cast<unsigned char>(left + ('a' - 'A'));
    }
    if (right >= 'A' && right <= 'Z') {
      right = static_cast<unsigned char>(right + ('a' - 'A'));
    }

    if (left != right) {
      return false;
    }
  }

  if (!seenNonAscii) {
    return true;
  }

  return EqualsIgnoreCase(Utf8ToWide(lhs), Utf8ToWide(rhs));
}

bool EqualsIgnoreCase(const std::wstring_view& lhs,
                      const std::wstring_view& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  if (lhs.size() >
      static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
    return false;
  }

  if (lhs.empty()) {
    return true;
  }

  const int length = static_cast<int>(lhs.size());
  const int result =
      CompareStringOrdinal(lhs.data(), length, rhs.data(), length, TRUE);
  return result == CSTR_EQUAL;
}

bool ContainsAnyToken(std::string_view haystack,
                      std::initializer_list<std::string_view> tokens) {
  const std::string upperHaystack = ToUpperAscii(haystack);
  for (const auto token : tokens) {
    if (upperHaystack.find(ToUpperAscii(token)) != std::string::npos) {
      return true;
    }
  }

  return false;
}

std::string Base64Encode(const std::vector<std::uint8_t>& bytes) {
  if (bytes.empty()) {
    return {};
  }

  static constexpr char kBase64Table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  const std::size_t size = bytes.size();
  std::string encoded(((size + 2) / 3) * 4, '\0');

  const std::uint8_t* src = bytes.data();
  char* dst = encoded.data();

  const std::size_t fullTripletCount = size / 3;
  for (std::size_t i = 0; i < fullTripletCount; ++i) {
    const std::uint32_t chunk = (static_cast<std::uint32_t>(src[0]) << 16u) |
                                (static_cast<std::uint32_t>(src[1]) << 8u) |
                                static_cast<std::uint32_t>(src[2]);

    dst[0] = kBase64Table[(chunk >> 18u) & 0x3fu];
    dst[1] = kBase64Table[(chunk >> 12u) & 0x3fu];
    dst[2] = kBase64Table[(chunk >> 6u) & 0x3fu];
    dst[3] = kBase64Table[chunk & 0x3fu];

    src += 3;
    dst += 4;
  }

  const std::size_t remaining = size - (fullTripletCount * 3);
  if (remaining == 1) {
    const std::uint32_t chunk = static_cast<std::uint32_t>(src[0]) << 16u;
    dst[0] = kBase64Table[(chunk >> 18u) & 0x3fu];
    dst[1] = kBase64Table[(chunk >> 12u) & 0x3fu];
    dst[2] = '=';
    dst[3] = '=';
  } else if (remaining == 2) {
    const std::uint32_t chunk = (static_cast<std::uint32_t>(src[0]) << 16u) |
                                (static_cast<std::uint32_t>(src[1]) << 8u);
    dst[0] = kBase64Table[(chunk >> 18u) & 0x3fu];
    dst[1] = kBase64Table[(chunk >> 12u) & 0x3fu];
    dst[2] = kBase64Table[(chunk >> 6u) & 0x3fu];
    dst[3] = '=';
  }

  return encoded;
}

std::string BytesToHexUpper(std::span<const std::uint8_t> bytes) {
  if (bytes.empty()) {
    return {};
  }

  static constexpr char kHexDigits[] = "0123456789ABCDEF";

  std::string hex;
  hex.resize(bytes.size() * 2);

  for (std::size_t i = 0; i < bytes.size(); ++i) {
    const std::uint8_t b = bytes[i];
    hex[(i * 2)] = kHexDigits[(b >> 4u) & 0x0fu];
    hex[(i * 2) + 1] = kHexDigits[b & 0x0fu];
  }

  return hex;
}
