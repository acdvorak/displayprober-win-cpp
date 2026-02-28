#pragma once

// TODO(acdvorak): Can this be removed?
// Source:
// https://github.com/Aleksoid1978/DisplayInfo/blob/ee114bc24/DisplayConfig/DisplayConfig.h#L23-L26
#ifndef _MSC_VER
#define WINVER 0x0605
#define NTDDI_VERSION NTDDI_WINBLUE
#endif

// This header needs to be imported first.
#include <Windows.h>

#include <map>
#include <string>

#include "CommonTypes.h"
#include "GdiPolyfills.h"

namespace gdi {

// Simplified aggregation of values pulled from Windows GDI `DISPLAYCONFIG_*`,
// DisplayID, and EDID.
//
// Populated by `QueryDisplayConfig()` and `DisplayConfigGetDeviceInfo()`
// from `WinUser.h`, which gives us:
//
// - `displayName` (`viewGdiDeviceName`)
// - `monitorName` (`monitorFriendlyDeviceName`)
// - `monitorDevicePath`
// - `width` and `height`
// - `scanLineOrdering` (progressive or interlaced)
// - `refreshRate`
// - `outputTechnology` (HDMI, DisplayPort, DVI, VGA, etc.)
// - `edidManufactureId` and `edidProductCodeId`
//
// Optionally enriched by GDI queries to provide color information like:
//
// - `colorEncoding`
// - `bitsPerColorChannel`
// - `activeColorMode`
// - `advancedColor` (HDR/WCG support and enablement) on Windows 10-11
//
// `advancedColor` contains:
//
// - `advancedColorSupported`
// - `advancedColorEnabled`
// - `advancedColorActive`
// - `highDynamicRangeSupported`
// - `highDynamicRangeUserEnabled`
// - `wideColorSupported`
// - `wideColorUserEnabled`
//
// The `GdiDisplayConfig` struct also has convenience methods
// `IsHdrSupported()` and `IsHdrEnabled()` that interrogate the properties of
// `advancedColor`.
struct GdiDisplayConfig {
  union {
    struct {
      /** A type of advanced color is supported */
      UINT32 advancedColorSupported : 1;
      /** A type of advanced color is enabled */
      UINT32 advancedColorEnabled : 1;
      /** Wide color gamut is enabled */
      UINT32 wideColorEnforced : 1;
      /** Advanced color is force disabled due to system/OS policy */
      UINT32 advancedColorForceDisabled : 1;
      UINT32 reserved : 28;
    };
    UINT32 value;
  } advancedColor;

  struct {
    union {
      struct {
        UINT32 advancedColorSupported : 1;
        UINT32 advancedColorActive : 1;
        UINT32 reserved1 : 1;
        UINT32 advancedColorLimitedByPolicy : 1;
        UINT32 highDynamicRangeSupported : 1;
        UINT32 highDynamicRangeUserEnabled : 1;
        UINT32 wideColorSupported : 1;
        UINT32 wideColorUserEnabled : 1;
        UINT32 reserved : 24;
      };
      UINT32 value;
    };

    DISPLAYCONFIG_ADVANCED_COLOR_MODE activeColorMode;
  } windows1124H2Colors;

  UINT32 width = 0;
  UINT32 height = 0;
  UINT32 bitsPerChannel = 0;
  DISPLAYCONFIG_COLOR_ENCODING colorEncoding;
  DISPLAYCONFIG_RATIONAL refreshRate;
  DISPLAYCONFIG_SCANLINE_ORDERING scanLineOrdering;
  DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY outputTechnology;

  // "Short Lived Identifier".
  //
  // Corresponds to: `DISPLAYCONFIG_SOURCE_DEVICE_NAME::viewGdiDeviceName`.
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

  // Friendly user-facing name, typically obtained from DisplayID or EDID.
  //
  // Corresponds to:
  // `DISPLAYCONFIG_TARGET_DEVICE_NAME::monitorFriendlyDeviceName`.
  //
  // Examples:
  //
  // - `"DELL ST2320L"`
  // - `"QCQ95S"` (Samsung S95C TV)
  // - `"SAMSUNG"` (some devices don't give us an actual model number)
  std::string friendly_name;

  // Persistent across reboots in the common case (same GPU/driver instance).
  //
  // Corresponds to: `DISPLAYCONFIG_ADAPTER_NAME::adapterDevicePath`
  std::string adapter_device_path;

  // Corresponds to `DISPLAYCONFIG_PATH_INFO.targetInfo.id`.
  std::uint32_t target_path_id = 0;

  // ✅ SECONDARY STABLE ID
  //
  // Typically stable across reboots and uniquely identifies the monitor
  // instance on that connection path.
  //
  // It is also very useful for correlating to EDID retrieval.
  //
  // Corresponds to: `DISPLAYCONFIG_TARGET_DEVICE_NAME::monitorDevicePath`.
  //
  // Examples:
  //
  // `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
  // `"\\\\?\\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
  DevicePath monitor_device_path;

  // Packed/encoded into 2 bytes.
  //
  // Corresponds to: `DISPLAYCONFIG_TARGET_DEVICE_NAME::edidManufactureId`
  UINT16 edidManufactureId = 0;

  // Packed/encoded into 2 bytes.
  //
  // Corresponds to: `DISPLAYCONFIG_TARGET_DEVICE_NAME::edidProductCodeId`
  UINT16 edidProductCodeId = 0;

  DISPLAYCONFIG_MODE_INFO modeTarget;

  bool hasAdvancedColorInfo = false;

  bool IsHdrSupported() const;
  bool IsHdrEnabled() const;
};

bool IsValidRefreshRate(const DISPLAYCONFIG_RATIONAL& rr);

std::map<ShortLivedIdentifier, GdiDisplayConfig> GetGdiDisplayConfigs();

}  // namespace gdi
