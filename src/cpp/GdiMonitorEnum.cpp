// Classic Win32 display monitor enumeration APIs (`HMONITOR`).
//
// Enumerates "display monitors" in the Windows desktop/virtual-screen
// coordinate space, including pseudo-monitors (e.g. mirroring drivers).

#include "GdiMonitorEnum.h"

#include <Windows.h>

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

static std::map<ShortLivedIdentifier, gdi::GdiMonitorInfo> gdi_monitor_infos;

// To continue the enumeration, return TRUE.
// To stop the enumeration, return FALSE.
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-monitorenumproc
BOOL CALLBACK EnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM) {
  if (hMonitor == 0) {
    // NOTE: An HMONITOR of 0 refers to a virtual monitor that spans all
    // physical monitors.
    //
    // Source:
    // https://webrtc.googlesource.com/src/+/c0fd2e0/modules/desktop_capture/win/screen_capture_utils.cc?pli=1#94
    //
    // I don't know if we can do anything useful with this information.
  }

  MONITORINFOEXW monitorInfoEx = {sizeof(monitorInfoEx)};
  if (!GetMonitorInfoW(hMonitor, &monitorInfoEx)) {
    return TRUE;  // Continue enumerating other monitors
  }

  ShortLivedIdentifier short_lived_identifier =
      WideToUtf8(monitorInfoEx.szDevice);

  gdi::GdiMonitorInfo& gdi = gdi_monitor_infos[short_lived_identifier];
  gdi.short_lived_identifier = short_lived_identifier;
  gdi.process_local_monitor_handle_ptr =
      reinterpret_cast<std::uintptr_t>(hMonitor);

  gdi.is_primary = ((monitorInfoEx.dwFlags & MONITORINFOF_PRIMARY) != 0);

  if (const auto get_dpi_for_monitor = ResolveGetDpiForMonitor()) {
    UINT dpiX, dpiY;
    if (S_OK ==
        get_dpi_for_monitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) {
      gdi.dpi_scale_percent = std::lround(dpiY * 100. / 96.);
    }
  }

  json_utils::PopulateRectangleIfZero(gdi.bounds, monitorInfoEx.rcMonitor);
  json_utils::PopulateRectangleIfZero(gdi.working_area, monitorInfoEx.rcWork);

  return TRUE;  // Continue enumerating other monitors
}

}  // namespace

namespace gdi {

std::map<ShortLivedIdentifier, gdi::GdiMonitorInfo> GetGdiMonitorInfos() {
  gdi_monitor_infos.clear();

  if (sys::is_win_10_v16070_or_newer()) {
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  } else if (sys::is_win_8dot1_or_newer()) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
  }

  EnumDisplayMonitors(nullptr, nullptr, EnumProc, 0);

  return gdi_monitor_infos;
}

std::map<ShortLivedIdentifier, GdiAdapterInfo> GetGdiAdapterInfoMap() {
  std::map<ShortLivedIdentifier, GdiAdapterInfo> gdi_adapter_infos;

  for (DWORD idx = 0;; ++idx) {
    DISPLAY_DEVICEW device = {};
    device.cb = sizeof(device);

    if (!EnumDisplayDevicesW(nullptr, idx, &device, 0)) {
      break;
    }

    if (device.DeviceName[0] == L'\0' || device.DeviceString[0] == L'\0') {
      continue;
    }

    GdiAdapterInfo gdi{};

    gdi.short_lived_identifier = WideToUtf8(device.DeviceName);
    gdi.adapter_friendly_name = WideToUtf8(device.DeviceString);
    gdi.adapter_hardware_id = WideToUtf8(device.DeviceID);
    gdi.adapter_registry_key = WideToUtf8(device.DeviceKey);

    gdi_adapter_infos.emplace(gdi.short_lived_identifier, gdi);
  }

  return gdi_adapter_infos;
}

}  // namespace gdi
