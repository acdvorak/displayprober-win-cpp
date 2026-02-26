
#pragma once

#include <Windows.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

std::string WideToUtf8(const wchar_t* value);
std::string WideToUtf8(const std::wstring_view& value);

std::wstring Utf8ToWide(std::string_view value);
std::wstring Utf8ToUpperWide(std::string_view value);

std::string ToUpperAscii(std::string_view value);

std::string Base64Encode(const std::vector<std::uint8_t>& bytes);

bool EqualsIgnoreCase(const std::string_view& lhs, const std::string_view& rhs);
bool EqualsIgnoreCase(const std::wstring_view& lhs,
                      const std::wstring_view& rhs);

bool ContainsAnyToken(std::string_view haystack,
                      std::initializer_list<std::string_view> tokens);

std::string Base64Encode(const std::vector<std::uint8_t>& bytes);
