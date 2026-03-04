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
// Populated by `EnumDisplayMonitors()` and `GetMonitorInfoW()` from
// `WinUser.h`.
struct BasicMonitorInfo {
  // Windows "monitor device name" from `MONITORINFOEX.szDevice`, populated by
  // `GetMonitorInfoW()`.
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

  // Raw `HMONITOR` handle (pointer value) from the `EnumDisplayMonitors()`
  // callback arg `hMonitor`.
  //
  // Value is process-local and NOT stable across topology changes
  // (device connects/disconnects).
  //
  // Example: `17749131`
  std::uintptr_t process_local_monitor_handle_ptr = 0;

  // In a given Windows desktop session, there is always exactly one "main"
  // display. All others - even ones that mirror the main display - are
  // "secondary".
  //
  // Value comes from `GetMonitorInfoW()`: bitmask of
  // `MONITORINFOEX.dwFlags & MONITORINFOF_PRIMARY`.
  //
  // Examples:
  // - `true` (main display)
  // - `false` (all other displays)
  bool is_primary = false;

  // DPI scaling percentage, as shown in the Windows Settings app under the
  // System > Display > "Scale and Layout" section (as of Windows 10 22H2).
  //
  // Value comes from
  // `GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)` in
  // `Shcore.dll`, computed with `round(dpiY * 100 / 96)`.
  //
  // Examples:
  // - `100` (96 DPI)
  // - `125` (120 DPI)
  // - `150` (144 DPI)
  std::optional<long> dpi_scale_percent;

  // Virtual-screen "pixel bounary" rectangle, including the taskbar.
  //
  // This is effectively the "full screen resolution" of the display.
  //
  // Value comes from `GetMonitorInfoW()` -> `MONITORINFOEX.rcMonitor`.
  //
  // Example: `{left: 0, top: 0, right: 1920, bottom: 1080}`.
  json::WinScreenRectangle bounds;

  // Virtual-screen "work area" rectangle, excluding the taskbar / docked bars.
  //
  // Value comes from  `GetMonitorInfoW()` -> `MONITORINFOEX.rcWork`.
  //
  // Example: `{left: 0, top: 0, right: 1920, bottom: 1040}` (equal to `bounds`
  // minus the height of the taskbar).
  json::WinScreenRectangle working_area;
};

// Gets all Windows display monitors, including invisible pseudo-monitors
// associated with the mirroring drivers.
std::map<ShortLivedIdentifier, basic::BasicMonitorInfo> GetBasicMonitorInfos();

}  // namespace basic
