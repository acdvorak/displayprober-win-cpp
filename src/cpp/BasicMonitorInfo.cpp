#include "BasicMonitorInfo.h"

#include <windows.h>

// Keep this separate from Windows.h, which needs to be included first.
#include <ShellScalingApi.h>

#include <iostream>
#include <map>
#include <string>

#include "JsonUtils.h"
#include "StringUtils.h"
#include "SysUtils.h"

namespace {

using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, MONITOR_DPI_TYPE, UINT*,
                                            UINT*);

GetDpiForMonitorFn ResolveGetDpiForMonitor() {
  static const GetDpiForMonitorFn cached = [] {
    HMODULE shcore = LoadLibraryW(L"Shcore.dll");
    if (shcore == nullptr) {
      return static_cast<GetDpiForMonitorFn>(nullptr);
    }
    return reinterpret_cast<GetDpiForMonitorFn>(
        GetProcAddress(shcore, "GetDpiForMonitor"));
  }();

  return cached;
}

static std::map<ShortLivedIdentifier, basic::BasicMonitorInfo>
    basic_monitor_infos;

// To continue the enumeration, return TRUE.
// To stop the enumeration, return FALSE.
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-monitorenumproc
BOOL CALLBACK EnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM) {
  MONITORINFOEXW monitorInfoEx = {sizeof(monitorInfoEx)};
  if (!GetMonitorInfoW(hMonitor, &monitorInfoEx)) {
    return TRUE;  // Continue enumerating other monitors
  }

  ShortLivedIdentifier monitorNameUtf8 = WideToUtf8(monitorInfoEx.szDevice);

  basic::BasicMonitorInfo& monitor = basic_monitor_infos[monitorNameUtf8];
  monitor.short_lived_identifier = monitorNameUtf8;
  monitor.hmonitor_id = reinterpret_cast<std::uintptr_t>(hMonitor);

  monitor.is_primary = ((monitorInfoEx.dwFlags & MONITORINFOF_PRIMARY) != 0);

  if (const auto get_dpi_for_monitor = ResolveGetDpiForMonitor()) {
    UINT dpiX, dpiY;
    if (S_OK ==
        get_dpi_for_monitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) {
      monitor.dpi_scale_percent = std::lround(dpiY * 100. / 96.);
    }
  }

  json_utils::PopulateRectangleIfZero(monitor.bounds, monitorInfoEx.rcMonitor);
  json_utils::PopulateRectangleIfZero(monitor.working_area,
                                      monitorInfoEx.rcWork);

  return TRUE;  // Continue enumerating other monitors
}

}  // namespace

namespace basic {

std::map<ShortLivedIdentifier, basic::BasicMonitorInfo> GetBasicMonitorInfos() {
  basic_monitor_infos.clear();

  if (sys::is_win_10_v16070_or_newer()) {
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  } else if (sys::is_win_8dot1_or_newer()) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
  }

  EnumDisplayMonitors(nullptr, nullptr, EnumProc, 0);

  return basic_monitor_infos;
}

}  // namespace basic
