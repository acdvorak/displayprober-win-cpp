#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

// Windows "monitor device name" from `MONITORINFOEX.szDevice`.
//
// ⚠️ NOT stable across device disconnects/reconnects.
//
// Examples:
//
// ```
// "\\\\.\\DISPLAY1"   (multi-monitor)
// "\\\\.\\DISPLAY2"   (multi-monitor)
// "\\\\.\\DISPLAY129" (Remote Desktop)
// "DISPLAY"           (single-monitor)
// "WinDisc"           (SSH console)
// ```
using ShortLivedIdentifier = std::string;

// Corresponds to the `InstanceName` field of the following WMI object classes:
//
// - `WmiMonitorBasicDisplayParams`
// - `WmiMonitorConnectionParams`
// - `WmiMonitorDescriptorMethods`
// - `WmiMonitorID`
// - `WmiMonitorListedSupportedSourceModes`
//
// Examples:
//
// - `"DISPLAY\\SAM73A5\\5&21e6c3e1&0&UID5243153_0"`
// - `"DISPLAY\\DELF023\\5&21e6c3e1&0&UID5243152_0"`
using WmiInstanceName = std::string;

// `WmiInstanceName` with the trailing `_N` removed.
//
// Examples:
//
// - `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153"`
// - `"DISPLAY\\DELF023\\5&21e6c3e1&0&UID5243152"`
using WmiJoinKey = std::string;

// Corresponds to: `DISPLAYCONFIG_TARGET_DEVICE_NAME.monitorDevicePath`.
//
// Examples:
//
// `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
//
// `"\\\\?\\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
using DevicePath = std::string;

constexpr std::uint8_t u8(std::optional<unsigned long long> v) {
  return static_cast<std::uint8_t>(v.value_or(0));
}
constexpr std::int8_t i8(std::optional<long long> v) {
  return static_cast<std::int8_t>(v.value_or(0));
}
constexpr std::uint16_t u16(std::optional<unsigned long long> v) {
  return static_cast<std::uint16_t>(v.value_or(0));
}
constexpr std::int16_t i16(std::optional<long long> v) {
  return static_cast<std::int16_t>(v.value_or(0));
}
constexpr std::uint32_t u32(std::optional<unsigned long long> v) {
  return static_cast<std::uint32_t>(v.value_or(0));
}
constexpr std::int32_t i32(std::optional<long long> v) {
  return static_cast<std::int32_t>(v.value_or(0));
}
constexpr std::uint64_t u64(std::optional<unsigned long long> v) {
  return static_cast<std::uint64_t>(v.value_or(0));
}
constexpr std::int64_t i64(std::optional<long long> v) {
  return static_cast<std::int64_t>(v.value_or(0));
}

template <typename E>
constexpr std::enable_if_t<std::is_enum_v<E>, std::uint8_t> u8(
    std::optional<E> v) {
  return static_cast<std::uint8_t>(v.value_or(static_cast<E>(0)));
}
template <typename E>
constexpr std::enable_if_t<std::is_enum_v<E>, std::int8_t> i8(
    std::optional<E> v) {
  return static_cast<std::int8_t>(v.value_or(static_cast<E>(0)));
}
template <typename E>
constexpr std::enable_if_t<std::is_enum_v<E>, std::uint16_t> u16(
    std::optional<E> v) {
  return static_cast<std::uint16_t>(v.value_or(static_cast<E>(0)));
}
template <typename E>
constexpr std::enable_if_t<std::is_enum_v<E>, std::int16_t> i16(
    std::optional<E> v) {
  return static_cast<std::int16_t>(v.value_or(static_cast<E>(0)));
}
template <typename E>
constexpr std::enable_if_t<std::is_enum_v<E>, std::uint32_t> u32(
    std::optional<E> v) {
  return static_cast<std::uint32_t>(v.value_or(static_cast<E>(0)));
}
template <typename E>
constexpr std::enable_if_t<std::is_enum_v<E>, std::int32_t> i32(
    std::optional<E> v) {
  return static_cast<std::int32_t>(v.value_or(static_cast<E>(0)));
}
template <typename E>
constexpr std::enable_if_t<std::is_enum_v<E>, std::uint64_t> u64(
    std::optional<E> v) {
  return static_cast<std::uint64_t>(v.value_or(static_cast<E>(0)));
}
template <typename E>
constexpr std::enable_if_t<std::is_enum_v<E>, std::int64_t> i64(
    std::optional<E> v) {
  return static_cast<std::int64_t>(v.value_or(static_cast<E>(0)));
}
