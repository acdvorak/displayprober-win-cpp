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
#include "DxgiOutputDevice.h"
#include "GdiDisplayConfig.h"
#include "JsonUtils.h"
#include "StringUtils.h"
#include "SysUtils.h"
#include "WmiQueries.h"
#include "gencode/acd-json.hpp"

using Microsoft::WRL::ComPtr;

namespace {

// Example `input` values:
//
// ```
// "\\\\.\\DISPLAY1"   (multi-monitor)
// "\\\\.\\DISPLAY2"   (multi-monitor)
// "\\\\.\\DISPLAY..." (multi-monitor)
// "\\\\.\\DISPLAY129" (Remote Desktop)
// "DISPLAY"           (single-monitor)
// "WinDisc"           (SSH console)
// ```
//
// Example return values:
//
// - `"DISPLAY1"`
// - `"DISPLAY2"`
// - `"DISPLAY129"`
// - `"DISPLAY"`
// - `"WinDisc"`
//
// If `input` does not match one of the above patterns, returns `""`.
std::string TryToExtractShortLivedIdentifier(std::string_view input) {
  constexpr std::string_view kPrefix = R"(\\.\)";
  if (input.starts_with(kPrefix)) {
    input.remove_prefix(kPrefix.size());
  }

  if (input == "WinDisc" || input.starts_with("DISPLAY")) {
    return std::string(input);
  }

  return "";
}

// Example `input` values:
//
// `"DISPLAY\\SAM73A5\\5&21e6c3e1&0&UID5243153_0"`
// `"DISPLAY\\DELF023\\5&21e6c3e1&0&UID5243152_0"`
// `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
// `"\\\\?\\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
//
// Example return values:
//
// - `"SAM73A5"`
// - `"DELF023"`
//
// If `input` does not match one of the above patterns, returns `""`.
std::string TryToExtractEdid7DigitIdentifier(std::string_view input) {
  auto is_valid_edid_id = [](const std::string_view value) {
    if (value.size() != 7) {
      return false;
    }

    for (const char ch : value) {
      const bool is_digit = (ch >= '0' && ch <= '9');
      const bool is_upper = (ch >= 'A' && ch <= 'Z');
      const bool is_lower = (ch >= 'a' && ch <= 'z');
      if (!is_digit && !is_upper && !is_lower) {
        return false;
      }
    }

    return true;
  };

  constexpr std::string_view kBackslashPrefix = "DISPLAY\\";
  if (input.starts_with(kBackslashPrefix)) {
    const std::size_t begin = kBackslashPrefix.size();
    const std::size_t end = input.find('\\', begin);
    if (end != std::string_view::npos) {
      const std::string_view candidate = input.substr(begin, end - begin);
      if (is_valid_edid_id(candidate)) {
        return std::string(candidate);
      }
    }
  }

  constexpr std::string_view kHashPrefix = R"(\\?\DISPLAY#)";
  if (input.starts_with(kHashPrefix)) {
    const std::size_t begin = kHashPrefix.size();
    const std::size_t end = input.find('#', begin);
    if (end != std::string_view::npos) {
      const std::string_view candidate = input.substr(begin, end - begin);
      if (is_valid_edid_id(candidate)) {
        return std::string(candidate);
      }
    }
  }

  return "";
}

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
    backups.push_back(TryToExtractEdid7DigitIdentifier(dc.monitor_device_path));
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
    backups.push_back(TryToExtractShortLivedIdentifier(dev.device_name));
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
  backups.push_back(TryToExtractShortLivedIdentifier(short_lived_identifier));

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

std::string BuildPrimaryPortKey(const gdi::GdiDisplayConfig& config) {
  const std::string gpu_identity =
      !config.adapter_instance_id.empty()
          ? config.adapter_instance_id
          : config.adapter_device_path.value_or("");
  if (gpu_identity.empty()) {
    return {};
  }

  auto tp_id = "0x" + IntsToHex(config.target_path_id);

  return std::format("acd_ppk:gpu_id={};tp_id={}", gpu_identity, tp_id);
}

std::string BuildMonitorPathKey(const DevicePath& monitor_device_path) {
  if (monitor_device_path.empty()) {
    return {};
  }

  return std::format("acd_mpk:mdp={}", monitor_device_path);
}

std::string BuildEdidKey(const std::optional<json::WinEdidInfo>& edid_info) {
  if (!edid_info) {
    return {};
  }

  const auto& info = *edid_info;
  if (!info.manufacturer_vid || !info.product_code_id ||
      !info.serial_number_id) {
    return {};
  }

  auto vid = ToUpperAscii(*info.manufacturer_vid);
  auto pid = "0x" + IntsToHex(static_cast<uint16_t>(*info.product_code_id));
  auto sn = "0x" + IntsToHex(static_cast<uint32_t>(*info.serial_number_id));

  return std::format("acd_edid:vid={};pid={};sn={}", vid, pid, sn);
}

void PopulateStableKeyFields(json::WinDisplay& json_obj) {
  std::vector<std::string> candidates;

  auto push_unique = [&candidates](const std::optional<std::string>& key) {
    if (!key || key->empty()) {
      return;
    }

    for (const auto& existing : candidates) {
      if (existing == *key) {
        return;
      }
    }

    candidates.push_back(*key);
  };

  push_unique(json_obj.primary_port_key);
  push_unique(json_obj.monitor_path_key);
  push_unique(json_obj.edid_key);

  if (candidates.empty()) {
    return;
  }

  json_obj.stable_id_candidates = candidates;
  json_obj.stable_id = candidates.front();

  if (json_obj.primary_port_key &&
      *json_obj.primary_port_key == candidates[0]) {
    json_obj.stable_id_source = json::StableIdSource::PRIMARY_PORT_KEY;
    return;
  }

  if (json_obj.monitor_path_key &&
      *json_obj.monitor_path_key == candidates[0]) {
    json_obj.stable_id_source = json::StableIdSource::MONITOR_PATH_KEY;
    return;
  }

  if (json_obj.edid_key && *json_obj.edid_key == candidates[0]) {
    json_obj.stable_id_source = json::StableIdSource::EDID_KEY;
  }
}

bool RectHasZeroWidthOrHeight(std::optional<RECT> rect) {
  if (!rect) {
    // Optional does not have a value, so it cannot be zero width/height.
    return false;
  }
  auto& r = *rect;
  if (r.top == 0 && r.bottom == 0) {
    return true;
  }
  if (r.left == 0 && r.right == 0) {
    return true;
  }
  return false;
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

    if (const std::string primary_port_key = BuildPrimaryPortKey(config);
        !primary_port_key.empty()) {
      json_obj.primary_port_key = primary_port_key;
    }

    // ✅ SECONDARY STABLE ID INPUT
    DevicePath monitor_device_path = config.monitor_device_path;

    if (!monitor_device_path.empty()) {
      json_obj.monitor_device_path = monitor_device_path;
      json_obj.monitor_path_key = BuildMonitorPathKey(monitor_device_path);
      json_obj.edid_info =
          wmi::GetWinEdidInfoFromDevicePath(monitor_device_path);
      if (const std::string edid_key = BuildEdidKey(json_obj.edid_info);
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
          static_cast<json::WinBitsPerColorChannel>(config.bitsPerChannel);
      json_obj.standard_color_info.color_encoding =
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
      std::cerr << "WARNING: DXGI device \"" << device.device_name
                << "\" is not attached to a desktop." << std::endl;
    }

    if (RectHasZeroWidthOrHeight(device.desktop_coordinates)) {
      std::cerr << "WARNING: DXGI device \"" << device.device_name
                << "\" has zero width or height." << std::endl;
    }

    json_obj.is_attached_to_desktop = device.is_attached_to_desktop;

    json_obj.rotation_deg =
        json_utils::DxgiRotationToJson(device.rotation_type);

    json_obj.standard_color_info.min_luminance = device.min_luminance;
    json_obj.standard_color_info.max_luminance = device.max_luminance;
    json_obj.standard_color_info.max_full_frame_luminance =
        device.max_full_frame_luminance;

    json_obj.standard_color_info.dxgi_color_space =
        json_utils::DxgiColorSpaceToJson(device.color_space);
  }

  PopulateStableKeyFields(json_obj);

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
    if (device.hmonitor_id != 0) {
      dxgi_output_devices_by_hmonitor[device.hmonitor_id] = device;
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

    if (basicMonitorInfo.hmonitor_id != 0) {
      dxgi_output_device = TryGetOptionalValue(dxgi_output_devices_by_hmonitor,
                                               basicMonitorInfo.hmonitor_id);
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
