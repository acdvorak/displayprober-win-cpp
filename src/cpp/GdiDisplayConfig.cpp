#include "GdiDisplayConfig.h"

// This header needs to be imported first.
#include <Windows.h>

#include <map>
#include <vector>

#include "GdiPolyfills.h"
#include "StringUtils.h"
#include "SysUtils.h"

namespace gdi {

static bool IsValidModeIndex(
    UINT32 modeInfoIdx, const std::vector<DISPLAYCONFIG_MODE_INFO>& modes) {
  return modeInfoIdx != DISPLAYCONFIG_PATH_MODE_IDX_INVALID &&
         modeInfoIdx < modes.size();
}

bool IsValidRefreshRate(const DISPLAYCONFIG_RATIONAL& rr) {
  // DisplayConfig sometimes reports a rate of 1 when the rate is not known
  return rr.Denominator != 0 && rr.Numerator / rr.Denominator > 1;
}

bool GdiDisplayConfig::IsHdrSupported() const {
  if (sys::is_win_11_v24H2_or_newer()) {
    return windows1124H2Colors.highDynamicRangeSupported;
  }

  return advancedColor.advancedColorSupported &&
         !advancedColor.wideColorEnforced &&
         !advancedColor.advancedColorForceDisabled;
}

bool GdiDisplayConfig::IsHdrEnabled() const {
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

std::map<ShortLivedIdentifier, GdiDisplayConfig> GetGdiDisplayConfigs() {
  std::map<ShortLivedIdentifier, GdiDisplayConfig> displayConfigs;

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
        return displayConfigs;
      }

      paths.resize(num_paths);
      modes.resize(num_modes);

      res = QueryDisplayConfig(flags, &num_paths, paths.data(), &num_modes,
                               modes.data(), nullptr);
    }
  } while (res == ERROR_INSUFFICIENT_BUFFER);

  if (res != ERROR_SUCCESS) {
    return displayConfigs;
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

    ShortLivedIdentifier deviceNameUtf8 = WideToUtf8(source.viewGdiDeviceName);

    GdiDisplayConfig& dc = displayConfigs[deviceNameUtf8];

    dc.short_lived_identifier = deviceNameUtf8;

    dc.outputTechnology = path.targetInfo.outputTechnology;

    if (IsValidModeIndex(path.sourceInfo.modeInfoIdx, modes)) {
      const auto& mode = modes[path.sourceInfo.modeInfoIdx];
      if (mode.infoType == DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE) {
        dc.width = mode.sourceMode.width;
        dc.height = mode.sourceMode.height;
      }
    }

    if (IsValidModeIndex(path.targetInfo.modeInfoIdx, modes)) {
      const auto& mode = modes[path.targetInfo.modeInfoIdx];

      dc.modeTarget = mode;
      dc.target_path_id = path.targetInfo.id;

      if (mode.infoType == DISPLAYCONFIG_MODE_INFO_TYPE_TARGET) {
        dc.refreshRate = mode.targetMode.targetVideoSignalInfo.vSyncFreq;
        dc.scanLineOrdering =
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
          dc.colorEncoding = color_info.colorEncoding;
          dc.bitsPerChannel = color_info.bitsPerColorChannel;
          dc.windows1124H2Colors.value = color_info.value;
          dc.windows1124H2Colors.activeColorMode = color_info.activeColorMode;
          dc.hasAdvancedColorInfo = true;
        }
      } else {
        DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO color_info = {
            {DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO,
             sizeof(color_info), mode.adapterId, mode.id},
            {}};

        res = DisplayConfigGetDeviceInfo(&color_info.header);
        if (res == ERROR_SUCCESS) {
          dc.colorEncoding = color_info.colorEncoding;
          dc.bitsPerChannel = color_info.bitsPerColorChannel;
          dc.advancedColor.value = color_info.value;
          dc.hasAdvancedColorInfo = true;
        }
      }
    }

    if (!IsValidRefreshRate(dc.refreshRate)) {
      dc.refreshRate = path.targetInfo.refreshRate;
      dc.scanLineOrdering = path.targetInfo.scanLineOrdering;

      if (!IsValidRefreshRate(dc.refreshRate)) {
        dc.refreshRate = {0, 1};
      }
    }

    DISPLAYCONFIG_TARGET_DEVICE_NAME target_dev_name = {
        {DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME, sizeof(target_dev_name),
         path.sourceInfo.adapterId, path.targetInfo.id},
        {},
    };

    res = DisplayConfigGetDeviceInfo(&target_dev_name.header);
    if (res == ERROR_SUCCESS) {
      dc.friendly_name = WideToUtf8(target_dev_name.monitorFriendlyDeviceName);
      dc.monitor_device_path = WideToUtf8(target_dev_name.monitorDevicePath);
      dc.edidManufactureId = target_dev_name.edidManufactureId;
      dc.edidProductCodeId = target_dev_name.edidProductCodeId;
    }

    DISPLAYCONFIG_ADAPTER_NAME adapter_name = {
        {DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME, sizeof(adapter_name),
         path.sourceInfo.adapterId, path.targetInfo.id},
        {},
    };

    res = DisplayConfigGetDeviceInfo(&adapter_name.header);
    if (res == ERROR_SUCCESS) {
      dc.adapter_device_path = WideToUtf8(adapter_name.adapterDevicePath);
    }
  }

  return displayConfigs;
}

}  // namespace gdi
