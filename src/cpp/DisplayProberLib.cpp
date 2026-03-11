#include "DisplayProberLib.h"

#include <wrl/client.h>

#include <format>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "CcdDisplayConfig.h"
#include "DisplayProberInternal.h"
#include "DxgiOutput.h"
#include "GdiMonitorEnum.h"
#include "JsonUtils.h"
#include "OptionalUtils.h"
#include "SetupApiDevice.h"
#include "StringUtils.h"
#include "SysUtils.h"
#include "WmiMonitor.h"
#include "gencode/acd-json.hpp"

using Microsoft::WRL::ComPtr;

namespace {

std::string GetFriendlyName(
    const size_t index, const size_t count,
    const ShortLivedIdentifier& short_lived_identifier,
    const gdi::GdiMonitorInfo& gdi_monitor_info,
    const std::optional<ccd::CcdDisplayConfig>& ccd_display_config,
    const std::optional<dxgi::DxgiOutputInfo>& dxgi_output_info) {
  std::vector<std::string> names;
  std::vector<std::string> backups;

  if (ccd_display_config) {
    auto& dc = *ccd_display_config;

    // Example values:
    //
    // - `"DELL ST2320L"`
    // - `"QCQ95S"` (Samsung S95C TV)
    // - `"SAMSUNG"` (some devices don't give us an actual model number)
    names.push_back(dc.display_friendly_name);

    // Example values:
    //
    // - `"SAM73A5"` (Samsung S95C TV)
    // - `"DELF023"` (Dell ST2320L monitor)
    backups.push_back(
        dp::internal::TryToExtractEdid7DigitIdentifier(dc.monitor_device_path));
  }

  if (dxgi_output_info) {
    auto& dev = *dxgi_output_info;

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

json::WinDisplay MergeDisplayDataToJson(
    const size_t index, const size_t count,
    const ShortLivedIdentifier& short_lived_identifier,
    const gdi::GdiMonitorInfo& gdi_monitor_info,
    const std::optional<ccd::CcdDisplayConfig>& ccd_display_config,
    const std::optional<dxgi::DxgiOutputInfo>& dxgi_output_info) {
  // Initialize all primitive fields to their default values.
  json::WinDisplay json_obj{};

  std::string friendly_name =
      GetFriendlyName(index, count, short_lived_identifier, gdi_monitor_info,
                      ccd_display_config, dxgi_output_info);

  json_obj.friendly_name = friendly_name;
  json_obj.short_lived_identifier = short_lived_identifier;

  json_obj.is_primary = gdi_monitor_info.is_primary;

  json_obj.bounds = gdi_monitor_info.bounds;
  json_obj.working_area = gdi_monitor_info.working_area;

  if (const auto pct = gdi_monitor_info.dpi_scale_percent.value_or(0);
      pct > 0) {
    json_obj.dpi_scaling_percent = static_cast<uint32_t>(pct);
  }

  // Initialize all primitive fields to their default values.
  json_obj.standard_color_info = {};

  if (ccd_display_config) {
    const auto& gdi = *ccd_display_config;

    if (HasValue(gdi.adapter_instance_id)) {
      json_obj.adapter_instance_id = gdi.adapter_instance_id;
    }

    if (HasValue(gdi.adapter_device_path)) {
      json_obj.adapter_device_path = *gdi.adapter_device_path;
    }

    if (gdi.adapter_info.has_value()) {
      const auto& info = *gdi.adapter_info;
      json_obj.adapter_friendly_name = info.adapter_friendly_name;
      json_obj.adapter_hardware_id = info.adapter_hardware_id;
      json_obj.adapter_registry_key = info.adapter_registry_key;
    }

    json_obj.target_path_id = gdi.target_path_id;

    if (const std::string primary_port_key =
            dp::internal::BuildPrimaryPortKey(gdi);
        !primary_port_key.empty()) {
      json_obj.primary_port_key = primary_port_key;
    }

    json_obj.monitor_instance_id = gdi.monitor_instance_id;
    json_obj.monitor_driver_key = gdi.monitor_driver_key;
    json_obj.monitor_registry_key = gdi.monitor_registry_key;

    // ✅ SECONDARY STABLE ID INPUT
    DevicePath monitor_device_path = gdi.monitor_device_path;

    if (!monitor_device_path.empty()) {
      json_obj.monitor_device_path = monitor_device_path;
      json_obj.monitor_path_key =
          dp::internal::BuildMonitorPathKey(monitor_device_path);
      json_obj.edid_info =
          wmi::GetWinEdidInfoFromDevicePath(monitor_device_path);

      if (json_obj.edid_info) {
        auto bytes =
            setupapi::GetEdidBytesFromMonitorDevicePath(monitor_device_path);
        if (bytes.has_value() && !bytes->empty()) {
          json_obj.edid_info->edid_bytes_base64 = Base64Encode(*bytes);
        }
      }

      if (const std::string edid_key =
              dp::internal::BuildEdidKey(json_obj.edid_info);
          !edid_key.empty()) {
        json_obj.edid_key = edid_key;
      }
    }

    json_obj.scan_line_ordering =
        json_utils::ScanLineOrderingToJson(gdi.scanLineOrdering);

    json_obj.standard_color_info.is_hdr_supported = gdi.IsHdrSupported();
    json_obj.standard_color_info.is_hdr_enabled = gdi.IsHdrEnabled();

    if (json_obj.bounds.width != gdi.width ||
        json_obj.bounds.height != gdi.height) {
      std::cerr << "WARNING: GdiMonitorInfo.bounds size does NOT match "
                   "CcdDisplayConfig size!"
                << std::endl;
    }

    if (ccd::IsValidRefreshRate(gdi.refreshRate)) {
      json_obj.refresh_rate_hz =
          static_cast<double>(gdi.refreshRate.Numerator) /
          static_cast<double>(gdi.refreshRate.Denominator);
      json_obj.refresh_rate_numerator = gdi.refreshRate.Numerator;
      json_obj.refresh_rate_denominator = gdi.refreshRate.Denominator;
    }

    json_obj.physical_connector_type =
        json_utils::OutputTechnologyToJson(gdi.outputTechnology);

    if (gdi.hasAdvancedColorInfo) {
      json_obj.standard_color_info.bits_per_channel =
          // TODO(acdvorak): Rename fields to lower_snake_case.
          static_cast<json::WinBitsPerColorChannel>(gdi.bitsPerChannel);
      json_obj.standard_color_info.color_encoding =
          // TODO(acdvorak): Rename fields to lower_snake_case.
          json_utils::ColorEncodingToJson(gdi.colorEncoding);

      // Initialize all primitive fields to their default values.
      json::WinAdvancedColorInfo advancedColorInfo{};
      if (sys::is_win_11_v24H2_or_newer()) {
        auto& colors = gdi.windows1124H2Colors;
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
        auto& colors = gdi.advancedColor;
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
            gdi.IsHdrSupported();
        advancedColorInfo.is_high_dynamic_range_user_enabled =
            gdi.IsHdrEnabled();
        advancedColorInfo.is_wide_color_supported =
            colors.wideColorEnforced != 0;
        advancedColorInfo.is_wide_color_user_enabled =
            colors.wideColorEnforced != 0;
      }

      json_obj.advanced_color_info = advancedColorInfo;
    }
  }

  if (dxgi_output_info) {
    auto& device = *dxgi_output_info;

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
      std::cerr << "WARNING: DxgiOutputInfo.bits_per_channel=" << dxgi_bpc
                << " differs from CcdDisplayConfig.bits_per_channel="
                << json_bpc << std::endl;
    }
  }

  dp::internal::PopulateStableKeyFields(json_obj);

  return json_obj;
}

}  // namespace

static void EnrichWithSetupApiData(ccd::CcdDisplayConfig& config) {
  if (HasValue(config.adapter_device_path)) {
    config.adapter_instance_id =
        setupapi::TryGetAdapterInstanceIdFromAdapterPath(
            config.adapter_device_path)
            .value_or("");
  }

  if (config.monitor_device_path.empty()) {
    return;
  }

  config.monitor_instance_id = setupapi::TryGetMonitorInstanceIdFromMonitorPath(
      config.monitor_device_path);
  if (!config.monitor_instance_id.has_value()) {
    return;
  }

  config.monitor_driver_key =
      setupapi::TryGetMonitorDriverKeyFromDeviceInstanceId(
          *config.monitor_instance_id);
  if (!config.monitor_driver_key.has_value()) {
    return;
  }

  config.monitor_registry_key =
      R"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\)" +
      *config.monitor_driver_key;
}

std::string GetDisplayProberJson() {
  // ═══════════════════════════════════════════════════════════
  // Tier 0 — Windows XP+ baseline
  // ═══════════════════════════════════════════════════════════
  // Source of truth for enumeration. This map will always contain at least one
  // value, even over remote SSH console sessions. For compatibility purposes,
  // Windows returns a "fake" virtual display named "WinDisc" over SSH.
  const std::map<ShortLivedIdentifier, gdi::GdiMonitorInfo> gdi_monitor_infos =
      gdi::GetGdiMonitorInfos();  // User32: EnumDisplayMonitors,
                                  // GetMonitorInfoW

  const std::map<ShortLivedIdentifier, gdi::GdiAdapterInfo> gdi_adapter_infos =
      gdi::GetGdiAdapterInfoMap();  // User32: EnumDisplayDevicesW

  // ═══════════════════════════════════════════════════════════
  // Tier 1 — Windows Vista+
  // ═══════════════════════════════════════════════════════════
  // Physical displays and RDP only. Will be empty on remote SSH consoles.
  const std::map<ShortLivedIdentifier, dxgi::DxgiOutputInfo> dxgi_output_infos =
      dxgi::GetDxgiOutputInfos();  // DXGI: CreateDXGIFactory,
                                   // IDXGIAdapter::EnumOutputs

  // ═══════════════════════════════════════════════════════════
  // Tier 2 — Windows 7+
  // ═══════════════════════════════════════════════════════════
  // Physical displays and RDP only. Will be empty on remote SSH consoles.
  std::map<ShortLivedIdentifier, ccd::CcdDisplayConfig> ccd_display_configs =
      ccd::GetCcdDisplayConfigs(
          gdi_adapter_infos);  // User32: QueryDisplayConfig,
                               // DisplayConfigGetDeviceInfo

  // Tier 2b: Enrich CCD data with SetupAPI device info (XP+ API, but depends on
  // CCD device paths)
  for (auto& [id, config] : ccd_display_configs) {
    EnrichWithSetupApiData(config);
  }

  std::map<std::uintptr_t, dxgi::DxgiOutputInfo>
      dxgi_output_devices_by_hmonitor;
  for (const auto& [_, device] : dxgi_output_infos) {
    if (device.process_local_monitor_handle_ptr != 0) {
      dxgi_output_devices_by_hmonitor[device.process_local_monitor_handle_ptr] =
          device;
    }
  }

  json::WinDisplayProberJson json_payload;

  json_payload.has_interactive_desktop = sys::HasInteractiveDesktop();
  json_payload.is_remote_desktop = sys::IsRdpSession();
  json_payload.is_virtual_machine = sys::IsVirtualMachine();

  size_t i = 0;
  for (const auto& [id, gdiMonitorInfo] : gdi_monitor_infos) {
    const auto gdi_display_config =
        TryGetOptionalValue(ccd_display_configs, id);

    std::optional<dxgi::DxgiOutputInfo> dxgiOutputInfo;

    if (gdiMonitorInfo.process_local_monitor_handle_ptr != 0) {
      dxgiOutputInfo =
          TryGetOptionalValue(dxgi_output_devices_by_hmonitor,
                              gdiMonitorInfo.process_local_monitor_handle_ptr);
    }

    if (!dxgiOutputInfo) {
      dxgiOutputInfo = TryGetOptionalValue(dxgi_output_infos, id);
    }

    json_payload.displays.push_back(MergeDisplayDataToJson(
        i++, gdi_monitor_infos.size(), id, gdiMonitorInfo, gdi_display_config,
        dxgiOutputInfo));
  }

  return json::json(json_payload).dump(2);
}
