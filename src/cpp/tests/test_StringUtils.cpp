#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "CommonTypes.h"
#include "StringUtils.h"

TEST_CASE("ToUpperAscii uppercases only ASCII letters") {
  CHECK(ToUpperAscii("abcXYZ123_-") == "ABCXYZ123_-");
  CHECK(ToUpperAscii("") == "");

  std::string nonAscii = "\xC3\xA4";
  CHECK(ToUpperAscii(nonAscii) == nonAscii);

  std::string longLower(16384, 'a');
  CHECK(ToUpperAscii(longLower) == std::string(16384, 'A'));
}

TEST_CASE("WideToUtf8 handles wchar_t pointer edge cases") {
  CHECK(WideToUtf8(static_cast<const wchar_t*>(nullptr)).empty());
  CHECK(WideToUtf8(L"").empty());
  CHECK(WideToUtf8(L"DisplayProber") == "DisplayProber");
}

TEST_CASE("WideToUtf8 and Utf8ToWide round-trip UTF-8 content") {
  const std::wstring wideInput = L"A\u03A9\u00DF";
  const std::string utf8 = WideToUtf8(std::wstring_view(wideInput));
  CHECK_FALSE(utf8.empty());
  CHECK(Utf8ToWide(utf8) == wideInput);

  CHECK(WideToUtf8(std::wstring_view{}).empty());
  CHECK(Utf8ToWide(std::string_view{}).empty());
}

TEST_CASE("Utf8ToUpperWide uppercases ASCII deterministically") {
  CHECK(Utf8ToUpperWide("abC123") == L"ABC123");
}

TEST_CASE("EqualsIgnoreCase compares ASCII case-insensitively") {
  CHECK(EqualsIgnoreCase("DisplayProber", "displayprober"));
  CHECK_FALSE(EqualsIgnoreCase("DisplayProber", "display_probe"));
  CHECK_FALSE(EqualsIgnoreCase("abc", "ab"));
}

TEST_CASE("EqualsIgnoreCase supports wide strings") {
  CHECK(
      EqualsIgnoreCase(std::wstring_view(L"TeSt"), std::wstring_view(L"test")));
  CHECK_FALSE(
      EqualsIgnoreCase(std::wstring_view(L"test"), std::wstring_view(L"tent")));
  CHECK(EqualsIgnoreCase(std::wstring_view{}, std::wstring_view{}));
}

TEST_CASE("ContainsAnySubstring matches case-insensitively") {
  CHECK(ContainsAnySubstring("NVIDIA GeForce RTX", {"geforce", "intel"}));
  CHECK(ContainsAnySubstring("AMD Radeon", {"ade"}));
  CHECK_FALSE(ContainsAnySubstring("AMD Radeon", {"intel", "nvidia"}));
  CHECK_FALSE(ContainsAnySubstring("", {"token"}));
  CHECK_FALSE(ContainsAnySubstring("value", {""}));
}

TEST_CASE("Base64Encode encodes known vectors") {
  CHECK(Base64Encode({}) == "");

  CHECK(Base64Encode(std::vector<std::uint8_t>{'f'}) == "Zg==");
  CHECK(Base64Encode(std::vector<std::uint8_t>{'f', 'o'}) == "Zm8=");
  CHECK(Base64Encode(std::vector<std::uint8_t>{'f', 'o', 'o'}) == "Zm9v");
  CHECK(Base64Encode(std::vector<std::uint8_t>{'f', 'o', 'o', 'b', 'a', 'r'}) ==
        "Zm9vYmFy");

  CHECK(Base64Encode(std::vector<std::uint8_t>{0x00, 0x01, 0xFF}) == "AAH/");
}

TEST_CASE("BytesToHexUpper and IntsToHex produce uppercase hex") {
  CHECK(BytesToHexUpper({}) == "");

  const std::array<std::uint8_t, 4> bytes = {0x00, 0x12, 0xAB, 0xFF};
  CHECK(BytesToHexUpper(bytes) == "0012ABFF");
  CHECK(IntsToHex<std::uint8_t>(std::span<const std::uint8_t>(bytes)) ==
        "0012ABFF");

  CHECK(IntsToHex(std::uint16_t{0x1234}) == "1234");
}

TEST_CASE("IntsToHex serializes single arguments") {
  CHECK(IntsToHex(i8(0x0F)) == "0F");
  CHECK(IntsToHex(u8(0x0F)) == "0F");
  CHECK(IntsToHex(i8(0xF0)) == "F0");
  CHECK(IntsToHex(u8(0xF0)) == "F0");
  CHECK(IntsToHex(i8(0xFF)) == "FF");
  CHECK(IntsToHex(u8(0xFF)) == "FF");
  CHECK(IntsToHex(i8(0xAB)) == "AB");
  CHECK(IntsToHex(u8(0xAB)) == "AB");
  CHECK(IntsToHex(i16(0x0F0F)) == "0F0F");
  CHECK(IntsToHex(u16(0x0F0F)) == "0F0F");
  CHECK(IntsToHex(i16(0xF0F0)) == "F0F0");
  CHECK(IntsToHex(u16(0xF0F0)) == "F0F0");
  CHECK(IntsToHex(i16(0xFFFF)) == "FFFF");
  CHECK(IntsToHex(u16(0xFFFF)) == "FFFF");
  CHECK(IntsToHex(i16(0xABCD)) == "ABCD");
  CHECK(IntsToHex(u16(0xABCD)) == "ABCD");
  CHECK(IntsToHex(i32(0x0F0F0F0F)) == "0F0F0F0F");
  CHECK(IntsToHex(u32(0x0F0F0F0F)) == "0F0F0F0F");
  CHECK(IntsToHex(i32(0xF0F0F0F0)) == "F0F0F0F0");
  CHECK(IntsToHex(u32(0xF0F0F0F0)) == "F0F0F0F0");
  CHECK(IntsToHex(i32(0xFFFFFFFF)) == "FFFFFFFF");
  CHECK(IntsToHex(u32(0xFFFFFFFF)) == "FFFFFFFF");
  CHECK(IntsToHex(i32(0x01234567)) == "01234567");
  CHECK(IntsToHex(u32(0x01234567)) == "01234567");
  CHECK(IntsToHex(i64(0x0F0F0F0F0F0F0F0F)) == "0F0F0F0F0F0F0F0F");
  CHECK(IntsToHex(u64(0x0F0F0F0F0F0F0F0F)) == "0F0F0F0F0F0F0F0F");
  CHECK(IntsToHex(i64(0xF0F0F0F0F0F0F0F0)) == "F0F0F0F0F0F0F0F0");
  CHECK(IntsToHex(u64(0xF0F0F0F0F0F0F0F0)) == "F0F0F0F0F0F0F0F0");
  CHECK(IntsToHex(i64(0xFFFFFFFFFFFFFFFF)) == "FFFFFFFFFFFFFFFF");
  CHECK(IntsToHex(u64(0xFFFFFFFFFFFFFFFF)) == "FFFFFFFFFFFFFFFF");
  CHECK(IntsToHex(i64(0x0123456789ABCDEF)) == "0123456789ABCDEF");
  CHECK(IntsToHex(u64(0x0123456789ABCDEF)) == "0123456789ABCDEF");
}

TEST_CASE("IntsToHex serializes multiple arguments") {
  CHECK(IntsToHex(u8(0x01), u8(0x20), u16(0x0304), u16(0x5060), u32(0x0708090A),
                  u32(0xB0C0D0E0), u64(0x0F01020304050607),
                  u64(0x8090A0B0C0D0E0F0)) ==
        "0120030450600708090AB0C0D0E00F010203040506078090A0B0C0D0E0F0");
}
