#include "DisplayProberLib.h"

#include <wrl/client.h>

#include <format>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "BasicMonitorInfo.h"
#include "DisplayProberInternal.h"
#include "DxgiOutputDevice.h"
#include "GdiDisplayConfig.h"
#include "JsonUtils.h"
#include "StringUtils.h"
#include "SysUtils.h"
#include "WmiQueries.h"
#include "gencode/acd-json.hpp"

using Microsoft::WRL::ComPtr;

namespace {

std::string GetFriendlyName(
    const size_t index, const size_t count,
    const ShortLivedIdentifier& short_lived_identifier,
    const basic::BasicMonitorInfo& basic_info,
    const std::optional<gdi::GdiDisplayConfig>& display_config,
    const std::optional<dxgi::DxgiOutputDevice>& output_device) {
  std::vector<std::string> names;
  std::vector<std::string> backups;

  if (display_config) {
    auto& dc = *display_config;

    // Example values:
    //
    // - `"DELL ST2320L"`
    // - `"QCQ95S"` (Samsung S95C TV)
    // - `"SAMSUNG"` (some devices don't give us an actual model number)
    names.push_back(dc.friendly_name);

    // Example values:
    //
    // - `"SAM73A5"` (Samsung S95C TV)
    // - `"DELF023"` (Dell ST2320L monitor)
    backups.push_back(
        dp::internal::TryToExtractEdid7DigitIdentifier(dc.monitor_device_path));
  }

  if (output_device) {
    auto& dev = *output_device;

    // Example values:
    //
    // - `"DISPLAY1"`
    // - `"DISPLAY2"` (multi-monitor)
    // - `"DISPLAY129"` (RDP)
    // - `"DISPLAY"` (single monitor)
    // - `"WinDisc"` (non-interactive remote SSH console session)
    backups.push_back(dp::internal::TryToExtractShortLivedIdentifier(
        dev.short_lived_identifier));
  }

  if (sys::IsRdpSession()) {
    if (count > 1) {
      names.push_back(std::format("Remote Desktop #{}", (index + 1)));
    } else {
      names.push_back("Remote Desktop");
    }
  }

  if (sys::IsVirtualMachine()) {
    if (count > 1) {
      names.push_back(std::format("Virtual Machine #{}", (index + 1)));
    } else {
      names.push_back("Virtual Machine");
    }
  }

  // Example values:
  //
  // - `"DISPLAY1"`
  // - `"DISPLAY2"` (multi-monitor)
  // - `"DISPLAY129"` (RDP)
  // - `"DISPLAY"` (single monitor)
  // - `"WinDisc"` (non-interactive remote SSH console session)
  backups.push_back(
      dp::internal::TryToExtractShortLivedIdentifier(short_lived_identifier));

  for (const std::string& value : names) {
    if (!value.empty()) {
      return value;
    }
  }

  for (const std::string& value : backups) {
    if (!value.empty()) {
      return value;
    }
  }

  return "UNKNOWN";
}

template <typename TMap, typename TKey>
const std::optional<typename TMap::mapped_type> TryGetOptionalValue(
    const TMap& map, const TKey& key) {
  const auto it = map.find(key);
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

json::WinDisplay MergeDisplayDataToJson(
    const size_t index, const size_t count,
    const ShortLivedIdentifier& short_lived_identifier,
    const basic::BasicMonitorInfo& basic_info,
    const std::optional<gdi::GdiDisplayConfig>& display_config,
    const std::optional<dxgi::DxgiOutputDevice>& output_device) {
  // Initialize all primitive fields to their default values.
  json::WinDisplay json_obj{};

  std::string friendly_name =
      GetFriendlyName(index, count, short_lived_identifier, basic_info,
                      display_config, output_device);

  json_obj.friendly_name = friendly_name;
  json_obj.short_lived_identifier = short_lived_identifier;

  json_obj.is_primary = basic_info.is_primary;

  json_obj.bounds = basic_info.bounds;
  json_obj.working_area = basic_info.working_area;

  if (const auto pct = basic_info.dpi_scale_percent.value_or(0); pct > 0) {
    json_obj.dpi_scaling_percent = static_cast<uint32_t>(pct);
  }

  // Initialize all primitive fields to their default values.
  json_obj.standard_color_info = {};

  if (display_config) {
    const auto& config = *display_config;

    if (!config.adapter_instance_id.empty()) {
      json_obj.adapter_instance_id = config.adapter_instance_id;
    }

    if (config.adapter_device_path.has_value()) {
      json_obj.adapter_device_path = *config.adapter_device_path;
    }
    json_obj.target_path_id = config.target_path_id;

    if (const std::string primary_port_key =
            dp::internal::BuildPrimaryPortKey(config);
        !primary_port_key.empty()) {
      json_obj.primary_port_key = primary_port_key;
    }

    // ✅ SECONDARY STABLE ID INPUT
    DevicePath monitor_device_path = config.monitor_device_path;

    if (!monitor_device_path.empty()) {
      json_obj.monitor_device_path = monitor_device_path;
      json_obj.monitor_path_key =
          dp::internal::BuildMonitorPathKey(monitor_device_path);
      json_obj.edid_info =
          wmi::GetWinEdidInfoFromDevicePath(monitor_device_path);
      if (const std::string edid_key =
              dp::internal::BuildEdidKey(json_obj.edid_info);
          !edid_key.empty()) {
        json_obj.edid_key = edid_key;
      }
    }

    json_obj.scan_line_ordering =
        json_utils::ScanLineOrderingToJson(config.scanLineOrdering);

    json_obj.standard_color_info.is_hdr_supported = config.IsHdrSupported();
    json_obj.standard_color_info.is_hdr_enabled = config.IsHdrEnabled();

    if (json_obj.bounds.width != config.width ||
        json_obj.bounds.height != config.height) {
      std::cerr << "WARNING: BasicMonitorInfo.bounds size does NOT match "
                   "GdiDisplayConfig size!"
                << std::endl;
    }

    if (gdi::IsValidRefreshRate(config.refreshRate)) {
      json_obj.refresh_rate_hz =
          static_cast<double>(config.refreshRate.Numerator) /
          static_cast<double>(config.refreshRate.Denominator);
      json_obj.refresh_rate_numerator = config.refreshRate.Numerator;
      json_obj.refresh_rate_denominator = config.refreshRate.Denominator;
    }

    json_obj.physical_connector_type =
        json_utils::OutputTechnologyToJson(config.outputTechnology);

    if (config.hasAdvancedColorInfo) {
      json_obj.standard_color_info.bits_per_channel =
          // TODO(acdvorak): Rename fields to lower_snake_case.
          static_cast<json::WinBitsPerColorChannel>(config.bitsPerChannel);
      json_obj.standard_color_info.color_encoding =
          // TODO(acdvorak): Rename fields to lower_snake_case.
          json_utils::ColorEncodingToJson(config.colorEncoding);

      // Initialize all primitive fields to their default values.
      json::WinAdvancedColorInfo advancedColorInfo{};
      if (sys::is_win_11_v24H2_or_newer()) {
        auto& colors = config.windows1124H2Colors;
        advancedColorInfo.is_advanced_color_supported =
            colors.advancedColorSupported != 0;
        advancedColorInfo.is_advanced_color_enabled =
            colors.advancedColorActive != 0;
        advancedColorInfo.is_wide_color_enforced = false;
        advancedColorInfo.is_advanced_color_force_disabled =
            colors.advancedColorLimitedByPolicy != 0 &&
            colors.advancedColorActive == 0;
        advancedColorInfo.is_advanced_color_active =
            colors.advancedColorActive != 0;
        advancedColorInfo.is_advanced_color_limited_by_policy =
            colors.advancedColorLimitedByPolicy != 0;
        advancedColorInfo.is_high_dynamic_range_supported =
            colors.highDynamicRangeSupported != 0;
        advancedColorInfo.is_high_dynamic_range_user_enabled =
            colors.highDynamicRangeUserEnabled != 0;
        advancedColorInfo.is_wide_color_supported =
            colors.wideColorSupported != 0;
        advancedColorInfo.is_wide_color_user_enabled =
            colors.wideColorUserEnabled != 0;
        advancedColorInfo.active_color_mode =
            json_utils::ActiveColorModeToJson(colors.activeColorMode);
      } else {
        auto& colors = config.advancedColor;
        advancedColorInfo.is_advanced_color_supported =
            colors.advancedColorSupported != 0;
        advancedColorInfo.is_advanced_color_enabled =
            colors.advancedColorEnabled != 0;
        advancedColorInfo.is_wide_color_enforced =
            colors.wideColorEnforced != 0;
        advancedColorInfo.is_advanced_color_force_disabled =
            colors.advancedColorForceDisabled != 0;
        advancedColorInfo.is_advanced_color_active =
            colors.advancedColorEnabled != 0;
        advancedColorInfo.is_advanced_color_limited_by_policy =
            colors.advancedColorForceDisabled != 0;
        advancedColorInfo.is_high_dynamic_range_supported =
            config.IsHdrSupported();
        advancedColorInfo.is_high_dynamic_range_user_enabled =
            config.IsHdrEnabled();
        advancedColorInfo.is_wide_color_supported =
            colors.wideColorEnforced != 0;
        advancedColorInfo.is_wide_color_user_enabled =
            colors.wideColorEnforced != 0;
      }

      json_obj.advanced_color_info = advancedColorInfo;
    }
  }

  if (output_device) {
    auto& device = *output_device;

    // This value MIGHT be `false` under the following conditions:
    //
    // - Unused connectors on the GPU:
    //   - Many drivers expose one IDXGIOutput per physical connector
    //     (HDMI/DP/DVI), even if nothing is plugged in.
    //   - Those "ports" can enumerate, but they are not part of the desktop, so
    //     AttachedToDesktop is false.
    //
    // - A monitor is connected but disabled in Display Settings:
    //   - Example: you have 2 monitors connected, but Windows is set to
    //     "Show only on 1" (or you've "Disconnect this display" for the other).
    //     That other output can still exist, but it is not attached, so false.
    if (!device.is_attached_to_desktop) {
      std::cerr << "WARNING: DXGI device \"" << device.short_lived_identifier
                << "\" is not attached to a desktop." << std::endl;
    }

    if (dp::internal::RectHasZeroWidthOrHeight(device.desktop_coordinates)) {
      std::cerr << "WARNING: DXGI device \"" << device.short_lived_identifier
                << "\" has zero width or height." << std::endl;
    }

    json_obj.is_attached_to_desktop = device.is_attached_to_desktop;

    json_obj.rotation_deg =
        json_utils::DxgiRotationToJson(device.rotation_type);

    json_obj.standard_color_info.min_luminance_nits = device.min_luminance_nits;
    json_obj.standard_color_info.max_luminance_nits = device.max_luminance_nits;
    json_obj.standard_color_info.max_full_frame_luminance_nits =
        device.max_full_frame_luminance_nits;

    json_obj.standard_color_info.dxgi_color_space =
        json_utils::DxgiColorSpaceToJson(device.color_space);

    // TODO(acdvorak): Make these values the same type (a uint8_t) and only
    // convert them to an enum when inserting into JSON.
    const std::uint8_t json_bpc = static_cast<std::uint8_t>(
        json_obj.standard_color_info.bits_per_channel.value_or(
            json::WinBitsPerColorChannel::VALUE_0));

    // TODO(acdvorak): Make these values the same type (a uint8_t) and only
    // convert them to an enum when inserting into JSON.
    const std::uint8_t dxgi_bpc =
        static_cast<std::uint8_t>(device.bits_per_channel.value_or(0));

    if (json_bpc > 0 && json_bpc != dxgi_bpc) {
      std::cerr << "WARNING: DxgiOutputDevice.bits_per_channel=" << dxgi_bpc
                << " differs from GdiDisplayConfig.bits_per_channel="
                << json_bpc << std::endl;
    }
  }

  dp::internal::PopulateStableKeyFields(json_obj);

  return json_obj;
}

}  // namespace

std::string GetDisplayProberJson() {
  // Source of truth for enumeration. This map will always contain at least one
  // value, even over remote SSH console sessions. For compatibility purposes,
  // Windows returns a "fake" virtual display named "WinDisc" over SSH.
  const std::map<ShortLivedIdentifier, basic::BasicMonitorInfo>
      basic_monitor_infos = basic::GetBasicMonitorInfos();

  // Physical displays and RDP only. Will be empty on remote SSH consoles.
  const std::map<ShortLivedIdentifier, gdi::GdiDisplayConfig>
      gdi_display_configs = gdi::GetGdiDisplayConfigs();

  // Physical displays and RDP only. Will be empty on remote SSH consoles.
  const std::map<ShortLivedIdentifier, dxgi::DxgiOutputDevice>
      dxgi_output_devices = dxgi::GetDxgiOutputDevices();

  std::map<std::uintptr_t, dxgi::DxgiOutputDevice>
      dxgi_output_devices_by_hmonitor;
  for (const auto& [_, device] : dxgi_output_devices) {
    if (device.process_local_monitor_handle_ptr != 0) {
      dxgi_output_devices_by_hmonitor[device.process_local_monitor_handle_ptr] =
          device;
    }
  }

  if (gdi_display_configs.size() > basic_monitor_infos.size()) {
    std::cerr << std::endl;
    std::cerr << "WARNING: gdi_display_configs.size="
              << gdi_display_configs.size()
              << " is greater than basic_monitor_infos.size="
              << basic_monitor_infos.size() << "." << std::endl;
    std::cerr << std::endl;
    std::cerr << "basic_monitor_infos:" << std::endl;
    for (const auto& [key, _] : basic_monitor_infos) {
      std::cerr << "    - " << key << std::endl;
    }
    std::cerr << std::endl;
    std::cerr << "gdi_display_configs:" << std::endl;
    for (const auto& [key, _] : gdi_display_configs) {
      auto name = _.friendly_name;
      auto adpt_path = _.adapter_device_path.value_or("nullopt");
      auto tp_id = _.target_path_id;
      auto mon_path = _.monitor_device_path;

      std::cerr << "    - " << key << std::endl;
      std::cerr << "        - friendly_name:       " << name << std::endl;
      std::cerr << "        - adapter_device_path: " << adpt_path << std::endl;
      std::cerr << "        - target_path_id:      " << tp_id << std::endl;
      std::cerr << "        - monitor_device_path: " << mon_path << std::endl;
    }
    std::cerr << std::endl;
  }

  if (dxgi_output_devices.size() > basic_monitor_infos.size()) {
    std::cerr << std::endl;
    std::cerr << "WARNING: dxgi_output_devices.size="
              << dxgi_output_devices.size()
              << " is greater than basic_monitor_infos.size="
              << basic_monitor_infos.size() << "." << std::endl;
    std::cerr << std::endl;
  }

  json::WinDisplayProberJson json_payload;

  json_payload.has_interactive_desktop = sys::HasInteractiveDesktop();
  json_payload.is_remote_desktop = sys::IsRdpSession();
  json_payload.is_virtual_machine = sys::IsVirtualMachine();

  size_t i = 0;
  for (const auto& [short_lived_identifier, basicMonitorInfo] :
       basic_monitor_infos) {
    const auto gdi_display_config =
        TryGetOptionalValue(gdi_display_configs, short_lived_identifier);

    std::optional<dxgi::DxgiOutputDevice> dxgi_output_device;

    if (basicMonitorInfo.process_local_monitor_handle_ptr != 0) {
      dxgi_output_device = TryGetOptionalValue(
          dxgi_output_devices_by_hmonitor,
          basicMonitorInfo.process_local_monitor_handle_ptr);
    }

    if (!dxgi_output_device) {
      dxgi_output_device =
          TryGetOptionalValue(dxgi_output_devices, short_lived_identifier);
    }

    json_payload.displays.push_back(MergeDisplayDataToJson(
        i++, basic_monitor_infos.size(), short_lived_identifier,
        basicMonitorInfo, gdi_display_config, dxgi_output_device));
  }

  return json::json(json_payload).dump(2);
}
