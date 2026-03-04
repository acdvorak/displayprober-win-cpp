export interface WinDisplayProberJson {
  displays: WinDisplay[];

  /**
   * This is a *session-level* value, not specific to an individual display.
   *
   * It will be `false` in all non-interactive sessions, such as:
   *
   * - SSH
   * - Remote console
   * - Headless server
   *
   * TODO(acdvorak): Clarify the difference between
   * {@link has_interactive_desktop} and {@link is_attached_to_desktop}.
   */
  has_interactive_desktop: boolean;

  /**
   * Indicates whether the current session is Microsoft Remote Desktop (RDP).
   *
   * This is a *session-level* value, not specific to an individual display.
   */
  is_remote_desktop: boolean;

  /**
   * Best-effort signal that this display is *probably* running in a VM guest.
   *
   * This is a *session-level* value, not specific to an individual display.
   */
  is_virtual_machine: boolean;
}

export interface WinDisplay {
  /**
   * Human-friendly name of the display.
   *
   * Value comes from one of the following sources, in descending order of
   * quality (i.e., the "best" available value is returned):
   *
   * 1. EDID "monitor descriptor" name (e.g., `"DELL ST2320L"`)
   * 2. `"Remote Desktop"` or `"Remote Desktop #N"` if RDP
   * 3. `"Virtual Machine"` or `"Virtual Machine #N"` if a VM
   * 4. 7-digit Windows EDID identifier (e.g., `"SAM73A5"` or `"DELF023"`)
   * 5. Short-lived Windows display number (e.g., `"DISPLAY1"`)
   *
   * Examples:
   *
   * - `"DELL ST2320L"`
   * - `"QCQ95S"`      // Samsung S95C TV
   * - `"SAMSUNG"`     // Some devices don't give us an actual model number
   * - `"SAM73A5"`     // Samsung S95C TV
   * - `"DELF023"`     // Dell ST2320L monitor
   * - `"DISPLAY1"`    // Primary monitor
   * - `"DISPLAY2"`    // Secondary monitor
   * - `"DISPLAY129"`  // RDP monitor
   * - `"DISPLAY"`     // Single monitor
   * - `"WinDisc"`     // Non-interactive remote SSH console session
   */
  friendly_name?: string | null;

  /**
   * Windows "monitor device name" from `MONITORINFOEX.szDevice`.
   *
   * ⚠️ NOT stable across device disconnects/reconnects.
   *
   * Examples:
   *
   * - `"\\\\.\\DISPLAY1"`   (multi-monitor)
   * - `"\\\\.\\DISPLAY2"`   (multi-monitor)
   * - `"\\\\.\\DISPLAY129"` (Remote Desktop)
   * - `"DISPLAY"`           (single-monitor)
   * - `"WinDisc"`           (SSH console)
   */
  short_lived_identifier: WinMonitorDeviceName;

  /**
   * Adapter Plug and Play instance ID (typically the GPU's unique ID).
   *
   * For a single GPU with two ports (e.g., one DisplayPort and one DVI), where
   * both ports are connected to an active monitor/TV, both displays will have
   * the same `adapter_instance_id`.
   *
   * TODO(acdvorak): Describe GPU vs Adapter vs Driver.
   *
   * More persistent than `adapter_device_path` across reboots and driver
   * churn.
   *
   * Examples:
   *
   * - `"PCI\\VEN_10DE&DEV_0DF8&SUBSYS_083510DE&REV_A1\\4&2B1C6285&0&0010"`
   */
  adapter_instance_id?: string | null;

  /**
   * Persistent across reboots in the common case (same GPU/driver instance).
   *
   * TODO(acdvorak): Describe GPU vs Adapter vs Driver.
   *
   * Corresponds to: `DISPLAYCONFIG_ADAPTER_NAME.adapterDevicePath`
   */
  adapter_device_path?: string | null;

  /**
   * Corresponds to `DISPLAYCONFIG_PATH_INFO.targetInfo.id`.
   *
   * @uint32
   */
  target_path_id?: number | null;

  /**
   * ✅ PRIMARY STABLE ID (when available)
   *
   * Value:
   *
   * ```
   * (adapter_instance_id ?? adapter_device_path) + target_path_id
   * ```
   */
  primary_port_key?: string | null;

  /**
   * Typically stable across reboots and uniquely identifies the monitor
   * instance on that connection path.
   *
   * It is also useful for correlating to EDID retrieval.
   *
   * Corresponds to: `DISPLAYCONFIG_TARGET_DEVICE_NAME.monitorDevicePath`.
   *
   * Examples:
   *
   * - `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
   * - `"\\\\?\\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
   */
  monitor_device_path?: string | null;

  /**
   * ✅ SECONDARY STABLE ID (when available)
   *
   * Deterministic key derived from {@link monitor_device_path}.
   */
  monitor_path_key?: string | null;

  /**
   * ✅ TERTIARY STABLE ID (when available)
   *
   * Deterministic monitor identity key based on EDID.
   *
   * Only emitted when manufacturer, product code, and serial are available.
   */
  edid_key?: string | null;

  /**
   * Effective stable ID after applying candidate ordering.
   */
  stable_id?: string | null;

  /**
   * Candidate stable keys ordered from strongest to weakest.
   *
   * 1. `primary_port_key`
   * 2. `monitor_path_key`
   * 3. `edid_key`
   *
   * TODO(acdvorak): Refactor
   */
  stable_id_candidates?: string[] | null;

  /**
   * Indicates which candidate produced {@link stable_id}.
   *
   * TODO(acdvorak): Refactor
   */
  stable_id_source?:
    | 'primary_port_key'
    | 'monitor_path_key'
    | 'edid_key'
    | null;

  is_primary: boolean;

  /**
   * This value MIGHT be `false` under the following conditions:
   *
   * - Unused connectors on the GPU:
   *   - Many drivers expose one IDXGIOutput per physical connector
   *     (HDMI/DP/DVI), even if nothing is plugged in.
   *   - Those "ports" can enumerate, but they are not part of the desktop, so
   *     AttachedToDesktop is false.
   *
   * - A monitor is connected but disabled in Display Settings:
   *   - Example: you have 2 monitors connected, but Windows is set to
   *     "Show only on 1" (or you've "Disconnect this display" for the other).
   *     That other output can still exist, but it is not attached, so false.
   *
   * TODO(acdvorak): Clarify the difference between
   * {@link has_interactive_desktop} and {@link is_attached_to_desktop}.
   */
  is_attached_to_desktop?: boolean | null;

  /**
   * The most common standard values are: `100 | 125 | 150 | 175 | 200`.
   *
   * The user can technically set any arbitrary value via registry hacks.
   *
   * @see https://superuser.com/questions/1328938/scale-100-on-windows-10/1328941#1328941
   * @see https://learn.microsoft.com/en-us/windows/win32/learnwin32/dpi-and-device-independent-pixels
   *
   * @uint32
   */
  dpi_scaling_percent?: number | null;

  /**
   * The full size and position of the display, *including* the taskbar and any
   * other areas that are not usable by maximized (non-fullscreen) applications.
   */
  bounds: WinScreenRectangle;

  /**
   * Available working area on the screen, *excluding* taskbars and other docked
   * windows.
   */
  working_area: WinScreenRectangle;

  /**
   * Progressive or interlaced.
   */
  scan_line_ordering?: WinScanLineOrder | null;

  /**
   * {@link refresh_rate_numerator} / {@link refresh_rate_denominator}.
   *
   * Examples:
   *
   * - `60`
   * - `120`
   * - `144`
   *
   * @double
   */
  refresh_rate_hz?: number | null;

  /** @uint32 */
  refresh_rate_numerator?: number | null;

  /** @uint32 */
  refresh_rate_denominator?: number | null;

  /** @uint8 */
  rotation_deg?: WinDisplayRotationDegrees | null;

  /**
   * Physical connector type, if applicable (HDMI, DVI, DisplayPort, etc.).
   */
  physical_connector_type?: WinDisplayConnectorType | null;

  standard_color_info: WinStandardColorInfo;
  advanced_color_info?: WinAdvancedColorInfo | null;

  edid_info?: WinEdidInfo | null;
}

/**
 * Rectangle payload used by Bounds and WorkingArea.
 */
export interface WinScreenRectangle {
  /** @int32 */
  x: number;
  /** @int32 */
  y: number;
  /** @uint32 */
  width: number;
  /** @uint32 */
  height: number;
  /** @int32 */
  left: number;
  /** @int32 */
  top: number;
  /** @int32 */
  right: number;
  /** @int32 */
  bottom: number;
}

export interface WinStandardColorInfo {
  is_hdr_supported: boolean;
  is_hdr_enabled: boolean;

  color_encoding?: WinColorEncoding | null;
  dxgi_color_space?: WinDxgiColorSpace | null;

  /** @uint8 */
  bits_per_channel?: WinBitsPerColorChannel | null;

  /**
   * In `nits` - i.e., luminance in candelas per square meter (`cd/m^2`).
   *
   * @double
   */
  min_luminance_nits?: number | null;

  /**
   * In `nits` - i.e., luminance in candelas per square meter (`cd/m^2`).
   *
   * @double
   */
  max_luminance_nits?: number | null;

  /**
   * Full-screen sustained luminance.
   *
   * In `nits` - i.e., luminance in candelas per square meter (`cd/m^2`).
   *
   * @double
   */
  max_full_frame_luminance_nits?: number | null;
}

export interface WinAdvancedColorInfo {
  is_advanced_color_supported: boolean;
  is_advanced_color_enabled: boolean;
  is_wide_color_enforced: boolean;
  is_advanced_color_force_disabled: boolean;
  is_advanced_color_active: boolean;
  is_advanced_color_limited_by_policy: boolean;
  is_high_dynamic_range_supported: boolean;
  is_high_dynamic_range_user_enabled: boolean;
  is_wide_color_supported: boolean;
  is_wide_color_user_enabled: boolean;

  active_color_mode?: WinActiveColorMode | null;
}

export interface WinEdidInfo {
  /**
   * Corresponds to: `DISPLAYCONFIG_TARGET_DEVICE_NAME.monitorDevicePath`.
   *
   * Examples:
   *
   * - `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
   * - `"\\\\?\\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
   */
  monitor_device_path: string;

  /**
   * Corresponds to the `InstanceName` field of these WMI object classes:
   *
   * - `WmiMonitorBasicDisplayParams`
   * - `WmiMonitorConnectionParams`
   * - `WmiMonitorDescriptorMethods`
   * - `WmiMonitorID`
   * - `WmiMonitorListedSupportedSourceModes`
   *
   * Examples:
   *
   * - `"DISPLAY\\SAM73A5\\5&21e6c3e1&0&UID5243153_0"`
   * - `"DISPLAY\\DELF023\\5&21e6c3e1&0&UID5243152_0"`
   */
  wmi_instance_name?: string | null;

  /**
   * Normalized join key, with trailing `_0` removed from
   * {@link wmi_instance_name}.
   *
   * E.g.:
   *
   * - `"DISPLAY\\SAM73A5\\5&21e6c3e1&0&UID5243153"`
   * - `"DISPLAY\\DELF023\\5&21e6c3e1&0&UID5243152"`
   */
  wmi_join_key: string;

  /** @double */
  max_horizontal_image_size_mm?: number | null;

  /** @double */
  max_vertical_image_size_mm?: number | null;

  /**
   * Raw numeric value that gets mapped to {@link WinDisplayConnectorType}.
   */
  video_output_technology_type?: WmiVideoOutputTechnology | null;

  /**
   * 3-letter Vendor ID (aka PnP ID).
   *
   * Examples:
   *
   * - `"SAM"` (Samsung)
   * - `"DEL"` (Dell)
   */
  manufacturer_vid?: string | null;

  /** @uint16 */
  product_code_id?: number | null;

  /** @uint32 */
  serial_number_id?: number | null;

  /**
   * Examples:
   *
   * - `"DELL ST2320L"`
   * - `"QCQ95S"`  // Samsung S95C TV
   * - `"SAMSUNG"` // Some devices don't give us an actual model number
   */
  user_friendly_name?: string | null;

  /** @uint8 */
  week_of_manufacture?: number | null;

  /** @uint16 */
  year_of_manufacture?: number | null;

  /**
   * Raw EDID bytes, Base64-encoded.
   */
  edid_bytes_base64?: string | null;
}

/**
 * On Windows, `System.Windows.Forms.Screen.DeviceName` is basically the Win
 * "monitor device name" string (the `szDevice` field of `MONITORINFOEX`,
 * filled by `GetMonitorInfo()`).
 *
 * In the common case, that comes out as the familiar GDI device path form:
 *
 * - `\\.\DISPLAY1`
 * - `\\.\DISPLAY2`
 * - ... (1-based index, can go higher than 2)
 *
 * WinForms historically also has a special-case fallback for "single monitor"
 * and "no multimon" where it sets the device name to:

 * - `DISPLAY` (no `\\.\` prefix)
 *
 * You can see both facts in the `WinForms` reference source:
 *
 * - Single-monitor path sets `"DISPLAY"`
 * - Multi-monitor path uses `MONITORINFOEX.szDevice`
 *
 * NOTE: Technically, this string may contain arbitrary non-printable
 * characters, though I have never observed that in the wild.
 *
 * @see https://learn.microsoft.com/en-us/dotnet/api/system.windows.forms.screen.devicename?view=windowsdesktop-10.0
 * @see https://github.com/dotnet/dotnet/blob/3bc68b106/src/winforms/src/System.Windows.Forms/System/Windows/Forms/Screen.cs#L63
 * @see https://github.com/dotnet/dotnet/blob/3bc68b106/src/winforms/src/System.Windows.Forms/System/Windows/Forms/Screen.cs#L78
 */
export type WinMonitorDeviceName =
  | '\\\\.\\DISPLAY1'
  | '\\\\.\\DISPLAY2'
  | '\\\\.\\DISPLAY3'
  | '\\\\.\\DISPLAY4'
  | '\\\\.\\DISPLAY5'
  | '\\\\.\\DISPLAY6'
  | '\\\\.\\DISPLAY7'
  | '\\\\.\\DISPLAY8'
  | '\\\\.\\DISPLAY9'
  | `\\\\.\\DISPLAY${number}`
  // Default/fallback value.
  | 'DISPLAY'
  // OpenSSH
  | 'WinDisc';

// export type WinRefreshRateHz =
//   | 23.976
//   | 24
//   | 25
//   | 29.97
//   | 30
//   | 47.952
//   | 48
//   | 50
//   | 59.94
//   | 60
//   | 71.928 // = 72/1.001
//   | 72
//   | 75
//   | 90 // less common than 75/100/120, but shows up
//   | 100 // very common for PAL-region TV modes and many monitors
//   | 119.88 // = 120/1.001
//   | 120
//   | 143.856 // = 144/1.001
//   | 144
//   | 165 // very common gaming monitor rate
//   | 170 // common on some 1440p gaming panels
//   | 180 // fairly common in newer gaming panels
//   | 200 // less common, but exists
//   | 240 // common for gaming; some TVs accept 240 input in certain modes
//   | (number & {});

/**
 * The most common standard values are: `100 | 125 | 150 | 175 | 200`.
 *
 * The user can technically set any arbitrary value via registry hacks.
 *
 * @see https://superuser.com/questions/1328938/scale-100-on-windows-10/1328941#1328941
 * @see https://learn.microsoft.com/en-us/windows/win32/learnwin32/dpi-and-device-independent-pixels
 */
// export type WinDpiScalingPercent = 100 | 125 | 150 | 175 | 200 | (number & {});

/**
 * Rotation in degrees.
 */
export type WinDisplayRotationDegrees = 0 | 90 | 180 | 270;

/**
 * Values with "embedded" in their names indicate that the graphics adapter's
 * video output device connects internally to the display device.
 *
 * In those cases, the `DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL` value is
 * redundant. The caller should ignore
 * `DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL` and just process the embedded
 * values, `DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED` and
 * `DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED`.
 *
 * An embedded display port is also known as an integrated display port or UDI.
 *
 * @see https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ne-wingdi-displayconfig_video_output_technology
 */
export type WinDisplayConnectorType =
  | 'other'
  | 'fake' // No attached desktop - e.g., SSH console session
  | 'rdp'
  | 'vga'
  | 'svideo'
  | 'composite_video'
  | 'component_video'
  | 'dvi'
  | 'hdmi'
  | 'lvds'
  | 'd_jpn'
  | 'sdi'
  | 'displayport_external'
  | 'displayport_embedded'
  | 'udi_external'
  | 'udi_embedded'
  | 'sdtvdongle'
  | 'miracast'
  | 'indirect_wired'
  | 'indirect_virtual'
  | 'displayport_usb_tunnel'
  | 'internal';

/**
 * Type of physical connector a video output device (on the display adapter)
 * uses to connect to an external display device.
 *
 * Known values for {@link WmiMonitorConnectionParams.VideoOutputTechnology}.
 *
 * @see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmdt/ne-d3dkmdt-_d3dkmdt_video_output_technology
 */
export enum WmiVideoOutputTechnology {
  uninitialized = -2,
  other = -1,

  /**
   * HD15 (VGA) connector.
   */
  vga = 0,

  /**
   * S-video connector.
   *
   * Aliases:
   * - `svideo_4pin` (4-pin S-video connector)
   * - `svideo_7pin` (7-pin S-video connector)
   */
  svideo = 1,

  /**
   * Composite video connectors.
   *
   * Aliases:
   * - `rf` (RF connector)
   * - `rca_3component` (a set of three RCA connectors)
   * - `bnc` (BNC connector)
   */
  composite_video = 2,

  /**
   * Component video connectors.
   */
  component_video = 3,

  /**
   * Digital Video Interface (DVI) connector.
   */
  dvi = 4,

  /**
   * High-Definition Multimedia Interface (HDMI) connector.
   */
  hdmi = 5,

  /**
   * Low Voltage Differential Swing (LVDS) or
   * Mobile Industry Processor Interface (MIPI) Digital Serial Interface (DSI)
   * connector.
   */
  lvds = 6,

  /**
   * D-Jpn connector.
   */
  d_jpn = 8,

  /**
   * SDI connector.
   */
  sdi = 9,

  /**
   * External DisplayPort connector.
   */
  displayport_external = 10,

  /**
   * Embedded DisplayPort (no external connector).
   */
  displayport_embedded = 11,

  /**
   * External Unified Display Interface (UDI) connector.
   */
  udi_external = 12,

  /**
   * Embedded Unified Display Interface (UDI) - no external connector.
   */
  udi_embedded = 13,

  /**
   * Dongle cable that supports SDTV.
   */
  sdtvdongle = 14,

  /**
   * Miracast connected session.
   *
   * For more info, see
   * [Wireless displays (Miracast)](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/wireless-displays--miracast-).
   *
   * Supported starting with Windows 8.1 (WDDM 1.3).
   */
  miracast = 15,

  /**
   * Wired indirect display device.
   *
   * Supported starting with Windows 10 version 1607 (WDDM 2.1).
   */
  indirect_wired = 16,

  /**
   * The video output device connects internally to a display device
   * (for example, the internal connection in a laptop computer).
   */
  internal = 0x80000000,
}

export type WinScanLineOrder =
  | 'unspecified'
  | 'progressive'
  | 'interlaced_upper_field_first'
  | 'interlaced_lower_field_first';

export type WinColorEncoding =
  | 'unspecified'
  | 'rgb'
  | 'ycbcr444'
  | 'ycbcr422'
  | 'ycbcr420';

/**
 * @see https://learn.microsoft.com/en-us/uwp/api/windows.graphics.display.advancedcolorkind?view=winrt-26100
 */
export type WinActiveColorMode = 'unspecified' | 'sdr' | 'wcg' | 'hdr';

export type WinBitsPerColorChannel =
  | 0 // Unspecified/unknown
  | 6
  | 8
  | 10
  | 12
  | 14
  | 16;

/**
 * @see https://learn.microsoft.com/en-us/windows/win32/api/dxgicommon/ne-dxgicommon-dxgi_color_space_type
 */
export type WinDxgiColorSpace =
  | 'rgb_full_g22_none_p709'
  | 'rgb_full_g10_none_p709'
  | 'rgb_studio_g22_none_p709'
  | 'rgb_studio_g22_none_p2020'
  | 'ycbcr_full_g22_none_p709_x601'
  | 'ycbcr_studio_g22_left_p601'
  | 'ycbcr_full_g22_left_p601'
  | 'ycbcr_studio_g22_left_p709'
  | 'ycbcr_full_g22_left_p709'
  | 'ycbcr_studio_g22_left_p2020'
  | 'ycbcr_full_g22_left_p2020'
  | 'rgb_full_g2084_none_p2020'
  | 'ycbcr_studio_g2084_left_p2020'
  | 'rgb_studio_g2084_none_p2020'
  | 'ycbcr_studio_g22_topleft_p2020'
  | 'ycbcr_studio_g2084_topleft_p2020'
  | 'rgb_full_g22_none_p2020'
  | 'ycbcr_studio_ghlg_topleft_p2020'
  | 'ycbcr_full_ghlg_topleft_p2020'
  | 'rgb_studio_g24_none_p709'
  | 'rgb_studio_g24_none_p2020'
  | 'ycbcr_studio_g24_left_p709'
  | 'ycbcr_studio_g24_left_p2020'
  | 'ycbcr_studio_g24_topleft_p2020'
  | 'reserved'
  | 'custom';
