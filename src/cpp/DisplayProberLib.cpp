#include "DisplayProberLib.h"

#include <wrl/client.h>

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
    auto& ccd = *ccd_display_config;

    // Example values:
    //
    // - `"DELL ST2320L"`
    // - `"QCQ95S"` (Samsung S95C TV)
    // - `"SAMSUNG"` (some devices don't give us an actual model number)
    names.push_back(ccd.display_friendly_name);

    // Example values:
    //
    // - `"SAM73A5"` (Samsung S95C TV)
    // - `"DELF023"` (Dell ST2320L monitor)
    backups.push_back(dp::internal::TryToExtractEdid7DigitIdentifier(
        ccd.monitor_device_path));
  }

  if (dxgi_output_info) {
    auto& dxgi = *dxgi_output_info;

    // Example values:
    //
    // - `"DISPLAY1"`
    // - `"DISPLAY2"` (multi-monitor)
    // - `"DISPLAY129"` (RDP)
    // - `"DISPLAY"` (single monitor)
    // - `"WinDisc"` (non-interactive remote SSH console session)
    backups.push_back(dp::internal::TryToExtractShortLivedIdentifier(
        dxgi.short_lived_identifier));
  }

  if (sys::IsRdpSession()) {
    if (count > 1) {
      names.push_back("Remote Desktop #" + std::to_string(index + 1));
    } else {
      names.push_back("Remote Desktop");
    }
  }

  if (sys::IsVirtualMachine()) {
    if (count > 1) {
      names.push_back("Virtual Machine #" + std::to_string(index + 1));
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
    const json::WinSetupApiDeviceCatalog& all_setup_api_devices,
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

  // Filter and correlate SetupAPI devices with this display using CCD
  // identifiers (adapter/monitor device paths and instance IDs).
  if (ccd_display_config) {
    const auto& ccd = *ccd_display_config;
    json::WinSetupApiDeviceCatalog matched{};

    for (const auto& adapter : all_setup_api_devices.adapters) {
      if ((HasValue(ccd.adapter_device_path) &&
           EqualsIgnoreCase(adapter.device_path_lowercase,
                            *ccd.adapter_device_path)) ||
          (HasValue(ccd.adapter_instance_id) && adapter.instance_id &&
           EqualsIgnoreCase(*adapter.instance_id, *ccd.adapter_instance_id))) {
        matched.adapters.push_back(adapter);
      }
    }

    for (const auto& monitor : all_setup_api_devices.monitors) {
      if ((!ccd.monitor_device_path.empty() &&
           EqualsIgnoreCase(monitor.device_path_lowercase,
                            ccd.monitor_device_path)) ||
          (HasValue(ccd.monitor_instance_id) && monitor.instance_id &&
           EqualsIgnoreCase(*monitor.instance_id, *ccd.monitor_instance_id))) {
        matched.monitors.push_back(monitor);
      }
    }

    if (!matched.adapters.empty() || !matched.monitors.empty()) {
      json_obj.setup_api_devices = {matched};
    }
  }

  if (const std::optional<long> dpi_pct = gdi_monitor_info.dpi_scale_percent;
      dpi_pct > 0) {
    json_obj.dpi_scaling_percent = u32(dpi_pct);
  }

  // Initialize all primitive fields to their default values.
  json_obj.standard_color_info = {};

  if (ccd_display_config) {
    const auto& ccd = *ccd_display_config;

    if (HasValue(ccd.adapter_instance_id)) {
      json_obj.adapter_instance_id = ccd.adapter_instance_id;
    }

    if (HasValue(ccd.adapter_device_path)) {
      json_obj.adapter_device_path = *ccd.adapter_device_path;
    }

    if (ccd.adapter_info.has_value()) {
      const auto& info = *ccd.adapter_info;
      json_obj.adapter_friendly_name = info.adapter_friendly_name;
      json_obj.adapter_hardware_id = info.adapter_hardware_id;
      json_obj.adapter_registry_key = info.adapter_registry_key;
    }

    json_obj.target_path_id = ccd.target_path_id;

    if (const std::string primary_port_key =
            dp::internal::BuildPrimaryPortKey(ccd);
        !primary_port_key.empty()) {
      json_obj.primary_port_key = primary_port_key;
    }

    json_obj.monitor_instance_id = ccd.monitor_instance_id;
    json_obj.monitor_driver_key = ccd.monitor_driver_key;
    json_obj.monitor_registry_key = ccd.monitor_registry_key;

    // ✅ SECONDARY STABLE ID INPUT
    DevicePath monitor_device_path = ccd.monitor_device_path;

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
        json_utils::ScanLineOrderingToJson(ccd.scanLineOrdering);

    json_obj.standard_color_info.is_hdr_supported = ccd.IsHdrSupported();
    json_obj.standard_color_info.is_hdr_enabled = ccd.IsHdrEnabled();

    if (json_obj.bounds.width != ccd.width ||
        json_obj.bounds.height != ccd.height) {
      std::cerr << "WARNING: GdiMonitorInfo.bounds size does NOT match "
                   "CcdDisplayConfig size!"
                << std::endl;
    }

    if (ccd::IsValidRefreshRate(ccd.refreshRate)) {
      json_obj.refresh_rate_hz =
          static_cast<double>(ccd.refreshRate.Numerator) /
          static_cast<double>(ccd.refreshRate.Denominator);
      json_obj.refresh_rate_numerator = ccd.refreshRate.Numerator;
      json_obj.refresh_rate_denominator = ccd.refreshRate.Denominator;
    }

    json_obj.physical_connector_type =
        json_utils::OutputTechnologyToJson(ccd.outputTechnology);

    if (ccd.hasAdvancedColorInfo) {
      json_obj.standard_color_info.bits_per_channel =
          static_cast<json::WinBitsPerColorChannel>(ccd.bitsPerColorChannel);
      json_obj.standard_color_info.color_encoding =
          json_utils::ColorEncodingToJson(ccd.colorEncoding);

      // Initialize all primitive fields to their default values.
      json::WinAdvancedColorInfo advancedColorInfo{};
      if (sys::is_win_11_v24H2_or_newer()) {
        auto& colors = ccd.windows1124H2Colors;
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
        auto& colors = ccd.advancedColor;
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
            ccd.IsHdrSupported();
        advancedColorInfo.is_high_dynamic_range_user_enabled =
            ccd.IsHdrEnabled();
        advancedColorInfo.is_wide_color_supported =
            colors.wideColorEnforced != 0;
        advancedColorInfo.is_wide_color_user_enabled =
            colors.wideColorEnforced != 0;
      }

      json_obj.advanced_color_info = advancedColorInfo;
    }
  }

  if (dxgi_output_info) {
    auto& dxgi = *dxgi_output_info;

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
    if (!dxgi.is_attached_to_desktop) {
      std::cerr << "WARNING: DXGI device \"" << dxgi.short_lived_identifier
                << "\" is not attached to a desktop." << std::endl;
    }

    if (dp::internal::RectHasZeroWidthOrHeight(dxgi.desktop_coordinates)) {
      std::cerr << "WARNING: DXGI device \"" << dxgi.short_lived_identifier
                << "\" has zero width or height." << std::endl;
    }

    json_obj.is_attached_to_desktop = dxgi.is_attached_to_desktop;

    json_obj.rotation_deg = json_utils::DxgiRotationToJson(dxgi.rotation_type);

    json_obj.standard_color_info.min_luminance_nits = dxgi.min_luminance_nits;
    json_obj.standard_color_info.max_luminance_nits = dxgi.max_luminance_nits;
    json_obj.standard_color_info.max_full_frame_luminance_nits =
        dxgi.max_full_frame_luminance_nits;

    json_obj.standard_color_info.dxgi_color_space =
        json_utils::DxgiColorSpaceToJson(dxgi.color_space);

    const std::uint8_t json_bpc =
        u8(json_obj.standard_color_info.bits_per_channel);

    const std::uint8_t dxgi_bpc = u8(dxgi.bits_per_channel);

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

static void EnrichWithSetupApiData(ccd::CcdDisplayConfig& display) {
  if (HasValue(display.adapter_device_path)) {
    display.adapter_instance_id =
        setupapi::TryGetAdapterInstanceIdFromAdapterPath(
            display.adapter_device_path);
  }

  if (display.monitor_device_path.empty()) {
    return;
  }

  display.monitor_instance_id =
      setupapi::TryGetMonitorInstanceIdFromMonitorPath(
          display.monitor_device_path);
  if (!HasValue(display.monitor_instance_id)) {
    return;
  }

  display.monitor_driver_key =
      setupapi::TryGetMonitorDriverKeyFromDeviceInstanceId(
          *display.monitor_instance_id);
  if (!HasValue(display.monitor_driver_key)) {
    return;
  }

  display.monitor_registry_key =
      R"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\)" +
      *display.monitor_driver_key;
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
      sys::is_win_vista_or_newer()
          ? dxgi::GetDxgiOutputInfos()  // DXGI: CreateDXGIFactory,
                                        // IDXGIAdapter::EnumOutputs
          : std::map<ShortLivedIdentifier, dxgi::DxgiOutputInfo>{};

  // ═══════════════════════════════════════════════════════════
  // Tier 2 — Windows 7+
  // ═══════════════════════════════════════════════════════════
  // Physical displays and RDP only. Will be empty on remote SSH consoles.
  std::map<ShortLivedIdentifier, ccd::CcdDisplayConfig> ccd_display_configs =
      sys::is_win_7_or_newer()
          ? ccd::GetCcdDisplayConfigs(
                gdi_adapter_infos)  // User32: QueryDisplayConfig,
                                    // DisplayConfigGetDeviceInfo
          : std::map<ShortLivedIdentifier, ccd::CcdDisplayConfig>{};

  // Tier 2b: Enrich CCD data with SetupAPI device info (XP+ API, but depends on
  // CCD device paths)
  for (auto& [id, display] : ccd_display_configs) {
    EnrichWithSetupApiData(display);
  }

  std::map<std::uintptr_t, dxgi::DxgiOutputInfo> dxgi_output_infos_by_hmonitor;
  for (const auto& [_, dxgi] : dxgi_output_infos) {
    if (dxgi.process_local_monitor_handle_ptr != 0) {
      dxgi_output_infos_by_hmonitor[dxgi.process_local_monitor_handle_ptr] =
          dxgi;
    }
  }

  json::WinDisplayProberJson json_payload;

  json_payload.has_interactive_desktop = sys::HasInteractiveDesktop();
  json_payload.is_remote_desktop = sys::IsRdpSession();
  json_payload.is_virtual_machine = sys::IsVirtualMachine();
  json_payload.all_setup_api_devices = setupapi::GetAllSetupApiDatas();

  size_t i = 0;
  for (const auto& [id, gdi] : gdi_monitor_infos) {
    const auto ccd_display_config =
        TryGetOptionalValue(ccd_display_configs, id);

    std::optional<dxgi::DxgiOutputInfo> dxgi;

    if (gdi.process_local_monitor_handle_ptr != 0) {
      dxgi = TryGetOptionalValue(dxgi_output_infos_by_hmonitor,
                                 gdi.process_local_monitor_handle_ptr);
    }

    if (!dxgi) {
      dxgi = TryGetOptionalValue(dxgi_output_infos, id);
    }

    json_payload.displays.push_back(MergeDisplayDataToJson(
        i++, gdi_monitor_infos.size(), id, gdi,
        json_payload.all_setup_api_devices, ccd_display_config, dxgi));
  }

  return json::json(json_payload).dump(2);
}
