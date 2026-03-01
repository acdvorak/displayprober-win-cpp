#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>

#include "CommonTypes.h"
#include "gencode/acd-json.hpp"

namespace basic {

// This is the simplest, most basic, most well-supported monitor information
// object available in Windows. The underlying APIs are old - they've been
// around since Windows 2000.
//
// Windows always returns at least one basic monitor object, even in
// non-interactive sessions (SSH, remote console, headless). This is to ensure
// compatibility with apps that expect one to always exist. It will be a
// simple, bare-bones, "default" monitor, 1024x768, named "WinDisc".
//
// Populated by GDI `EnumDisplayMonitors()` and `GetMonitorInfoW()` from
// `WinUser.h`.
struct BasicMonitorInfo {
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
  ShortLivedIdentifier short_lived_identifier;

  std::uintptr_t hmonitor_id = 0;

  bool is_primary = false;

  std::optional<long> dpi_scale_percent;
  json::WinScreenRectangle bounds;
  json::WinScreenRectangle working_area;
};

// Gets all Windows display monitors, including invisible pseudo-monitors
// associated with the mirroring drivers.
std::map<ShortLivedIdentifier, basic::BasicMonitorInfo> GetBasicMonitorInfos();

}  // namespace basic
