// Topology graph of the current display configuration: "paths" + "modes".
//
// Gives you per-target details like EDID-derived identifiers (not raw EDID
// bytes), friendly names, and device paths.

#include "CcdDisplayConfig.h"

// This header needs to be imported first.
#include <Windows.h>

#include <map>
#include <vector>

#include "CcdPolyfills.h"
#include "StringUtils.h"
#include "SysUtils.h"

namespace ccd {

static bool IsValidModeIndex(
    UINT32 modeInfoIdx, const std::vector<DISPLAYCONFIG_MODE_INFO>& modes) {
  return modeInfoIdx != DISPLAYCONFIG_PATH_MODE_IDX_INVALID &&
         modeInfoIdx < modes.size();
}

bool IsValidRefreshRate(const DISPLAYCONFIG_RATIONAL& rr) {
  // DisplayConfig sometimes reports a rate of 1 when the rate is not known
  return rr.Denominator != 0 && rr.Numerator / rr.Denominator > 1;
}

bool CcdDisplayConfig::IsHdrSupported() const {
  if (sys::is_win_11_v24H2_or_newer()) {
    return windows1124H2Colors.highDynamicRangeSupported;
  }

  return advancedColor.advancedColorSupported &&
         !advancedColor.wideColorEnforced &&
         !advancedColor.advancedColorForceDisabled;
}

bool CcdDisplayConfig::IsHdrEnabled() const {
  if (sys::is_win_11_v24H2_or_newer()) {
    return windows1124H2Colors.highDynamicRangeSupported &&
           windows1124H2Colors.activeColorMode ==
               DISPLAYCONFIG_ADVANCED_COLOR_MODE::
                   DISPLAYCONFIG_ADVANCED_COLOR_MODE_HDR;
  }

  return advancedColor.advancedColorEnabled &&
         !advancedColor.wideColorEnforced &&
         !advancedColor.advancedColorForceDisabled;
}

std::map<ShortLivedIdentifier, CcdDisplayConfig> GetCcdDisplayConfigs(
    const std::map<ShortLivedIdentifier, GdiAdapterInfo>& adapter_info_map) {
  std::map<ShortLivedIdentifier, CcdDisplayConfig> ccd_display_configs;

  UINT32 num_paths;
  UINT32 num_modes;
  std::vector<DISPLAYCONFIG_PATH_INFO> paths;
  std::vector<DISPLAYCONFIG_MODE_INFO> modes;
  LONG res;

  // The display configuration could change between the call to
  // GetDisplayConfigBufferSizes and the call to QueryDisplayConfig, so call
  // them in a loop until the correct buffer size is chosen
  do {
    UINT32 flags = QDC_ONLY_ACTIVE_PATHS;

    res = GetDisplayConfigBufferSizes(flags, &num_paths, &num_modes);
    if (res == ERROR_SUCCESS) {
      if (num_paths == 0 || num_modes == 0) {
        return ccd_display_configs;
      }

      paths.resize(num_paths);
      modes.resize(num_modes);

      res = QueryDisplayConfig(flags, &num_paths, paths.data(), &num_modes,
                               modes.data(), nullptr);
    }
  } while (res == ERROR_INSUFFICIENT_BUFFER);

  if (res != ERROR_SUCCESS) {
    return ccd_display_configs;
  }

  // num_paths and num_modes could decrease in a loop
  paths.resize(num_paths);
  modes.resize(num_modes);

  for (const auto& path : paths) {
    // Send a GET_SOURCE_NAME request
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source = {
        {DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME, sizeof(source),
         path.sourceInfo.adapterId, path.sourceInfo.id},
        {},
    };

    res = DisplayConfigGetDeviceInfo(&source.header);
    if (res != ERROR_SUCCESS) {
      continue;
    }

    ShortLivedIdentifier short_lived_identifier =
        WideToUtf8(source.viewGdiDeviceName);

    CcdDisplayConfig& display = ccd_display_configs[short_lived_identifier];

    display.short_lived_identifier = short_lived_identifier;
    display.target_path_id = path.targetInfo.id;
    display.outputTechnology = path.targetInfo.outputTechnology;

    if (IsValidModeIndex(path.sourceInfo.modeInfoIdx, modes)) {
      const auto& mode = modes[path.sourceInfo.modeInfoIdx];
      if (mode.infoType == DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE) {
        display.width = mode.sourceMode.width;
        display.height = mode.sourceMode.height;
      }
    }

    if (IsValidModeIndex(path.targetInfo.modeInfoIdx, modes)) {
      const auto& mode = modes[path.targetInfo.modeInfoIdx];

      display.modeTarget = mode;

      if (mode.infoType == DISPLAYCONFIG_MODE_INFO_TYPE_TARGET) {
        display.refreshRate = mode.targetMode.targetVideoSignalInfo.vSyncFreq;
        display.scanLineOrdering =
            mode.targetMode.targetVideoSignalInfo.scanLineOrdering;
      }

      if (sys::is_win_11_v24H2_or_newer()) {
        DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO_2 color_info = {
            {static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(
                 DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO_2),
             sizeof(color_info), mode.adapterId, mode.id},
            {}};
        res = DisplayConfigGetDeviceInfo(&color_info.header);
        if (res == ERROR_SUCCESS) {
          display.colorEncoding = color_info.colorEncoding;
          display.bitsPerChannel = color_info.bitsPerColorChannel;
          display.windows1124H2Colors.value = color_info.value;
          display.windows1124H2Colors.activeColorMode =
              color_info.activeColorMode;
          display.hasAdvancedColorInfo = true;
        }
      } else {
        DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO color_info = {
            {DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO,
             sizeof(color_info), mode.adapterId, mode.id},
            {}};
        res = DisplayConfigGetDeviceInfo(&color_info.header);
        if (res == ERROR_SUCCESS) {
          display.colorEncoding = color_info.colorEncoding;
          display.bitsPerChannel = color_info.bitsPerColorChannel;
          display.advancedColor.value = color_info.value;
          display.hasAdvancedColorInfo = true;
        }
      }
    }

    if (!IsValidRefreshRate(display.refreshRate)) {
      display.refreshRate = path.targetInfo.refreshRate;
      display.scanLineOrdering = path.targetInfo.scanLineOrdering;

      if (!IsValidRefreshRate(display.refreshRate)) {
        display.refreshRate = {0, 1};
      }
    }

    DISPLAYCONFIG_TARGET_DEVICE_NAME target_dev_name = {
        {DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME, sizeof(target_dev_name),
         path.sourceInfo.adapterId, path.targetInfo.id},
        {},
    };
    res = DisplayConfigGetDeviceInfo(&target_dev_name.header);
    if (res == ERROR_SUCCESS) {
      display.display_friendly_name =
          WideToUtf8(target_dev_name.monitorFriendlyDeviceName);
      display.monitor_device_path =
          WideToUtf8(target_dev_name.monitorDevicePath);
      display.edidManufactureId = target_dev_name.edidManufactureId;
      display.edidProductCodeId = target_dev_name.edidProductCodeId;
    }

    DISPLAYCONFIG_ADAPTER_NAME adapter_name = {
        {DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME, sizeof(adapter_name),
         path.sourceInfo.adapterId, path.targetInfo.id},
        {},
    };
    res = DisplayConfigGetDeviceInfo(&adapter_name.header);
    if (res == ERROR_SUCCESS) {
      display.adapter_device_path = WideToUtf8(adapter_name.adapterDevicePath);
    }

    // TODO(acdvorak): Replace all `.end()` iterator finds with
    // `TryGetOptionalValue()`.
    const auto adapter_name_it = adapter_info_map.find(short_lived_identifier);
    if (adapter_name_it != adapter_info_map.end()) {
      display.adapter_info = adapter_name_it->second;
    }
  }

  return ccd_display_configs;
}

}  // namespace ccd
