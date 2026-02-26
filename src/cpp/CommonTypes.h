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

// Corresponds to WMI object's `InstanceName`.
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
using NormalizedJoinKey = std::string;

// Corresponds to: `DISPLAYCONFIG_TARGET_DEVICE_NAME.monitorDevicePath`.
//
// Examples:
//
// `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
//
// `"\\\\?\\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
using DevicePath = std::string;
