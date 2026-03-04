#pragma once

#include <cstdint>
#include <string>
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

// clang-format off
constexpr std::uint8_t  u8 (unsigned long long v) { return static_cast<std::uint8_t>(v); }
constexpr std::int8_t   i8 (long long v)          { return static_cast<std::int8_t>(v); }
constexpr std::uint16_t u16(unsigned long long v) { return static_cast<std::uint16_t>(v); }
constexpr std::int16_t  i16(long long v)          { return static_cast<std::int16_t>(v); }
constexpr std::uint32_t u32(unsigned long long v) { return static_cast<std::uint32_t>(v); }
constexpr std::int32_t  i32(long long v)          { return static_cast<std::int32_t>(v); }
constexpr std::uint64_t u64(unsigned long long v) { return static_cast<std::uint64_t>(v); }
constexpr std::int64_t  i64(long long v)          { return static_cast<std::int64_t>(v); }
// clang-format on
