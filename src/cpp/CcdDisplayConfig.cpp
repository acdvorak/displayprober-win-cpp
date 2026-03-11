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
#include "SetupApiDevice.h"
#include "StringUtils.h"
#include "SysUtils.h"

namespace ccd {

namespace {

std::map<ShortLivedIdentifier, GdiAdapterInfo>
GetShortLivedIdToAdapterInfoMap() {
  std::map<ShortLivedIdentifier, GdiAdapterInfo> adapters;

  for (DWORD idx = 0;; ++idx) {
    DISPLAY_DEVICEW dd = {};
    dd.cb = sizeof(dd);

    if (!EnumDisplayDevicesW(nullptr, idx, &dd, 0)) {
      break;
    }

    if (dd.DeviceName[0] == L'\0' || dd.DeviceString[0] == L'\0') {
      continue;
    }

    GdiAdapterInfo info{};

    info.short_lived_identifier = WideToUtf8(dd.DeviceName);
    info.adapter_friendly_name = WideToUtf8(dd.DeviceString);
    info.adapter_hardware_id = WideToUtf8(dd.DeviceID);
    info.adapter_registry_key = WideToUtf8(dd.DeviceKey);

    adapters.emplace(info.short_lived_identifier, info);
  }

  return adapters;
}

}  // namespace

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

std::map<ShortLivedIdentifier, CcdDisplayConfig> GetCcdDisplayConfigs() {
  std::map<ShortLivedIdentifier, CcdDisplayConfig> displayConfigs;

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

  const auto id_to_adapter_info_map = GetShortLivedIdToAdapterInfoMap();

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

    CcdDisplayConfig& dc = displayConfigs[short_lived_identifier];

    dc.short_lived_identifier = short_lived_identifier;
    dc.target_path_id = path.targetInfo.id;
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
      dc.display_friendly_name =
          WideToUtf8(target_dev_name.monitorFriendlyDeviceName);
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

      const auto adapter_instance_id =
          setupapi::TryGetAdapterInstanceIdFromAdapterPath(
              dc.adapter_device_path);
      if (adapter_instance_id.has_value()) {
        dc.adapter_instance_id = *adapter_instance_id;
      }
    }

    const auto adapter_name_it =
        id_to_adapter_info_map.find(short_lived_identifier);
    if (adapter_name_it != id_to_adapter_info_map.end()) {
      dc.adapter_info = adapter_name_it->second;
    }

    if (HasValue(dc.monitor_device_path)) {
      dc.monitor_instance_id = setupapi::TryGetMonitorInstanceIdFromMonitorPath(
          dc.monitor_device_path);
    }

    if (HasValue(dc.monitor_instance_id)) {
      dc.monitor_driver_key =
          setupapi::TryGetMonitorDriverKeyFromDeviceInstanceId(
              *dc.monitor_instance_id);
      if (HasValue(dc.monitor_driver_key)) {
        dc.monitor_registry_key =
            R"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\)" +
            *dc.monitor_driver_key;
      }
    }
  }

  return displayConfigs;
}

}  // namespace ccd
