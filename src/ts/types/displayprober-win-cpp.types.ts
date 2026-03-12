export interface WinDisplayProberJson {
  displays: WinDisplay[];

  all_setup_api_devices: WinSetupApiDeviceCatalog;

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

/**
 * An individual monitor or virtual display attached to a Windows PC.
 */
export interface WinDisplay {
  /**
   * Human-friendly name of the display.
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
   *
   * Value comes from one of the following sources, in descending order of
   * quality (i.e., the "best" available value is returned):
   *
   * 1. EDID "monitor descriptor" name (e.g., `"DELL ST2320L"`)
   * 2. `"Remote Desktop"` or `"Remote Desktop #N"` if RDP
   * 3. `"Virtual Machine"` or `"Virtual Machine #N"` if a VM
   * 4. 7-digit Windows EDID identifier (e.g., `"SAM73A5"` or `"DELF023"`)
   * 5. Short-lived Windows display number (e.g., `"DISPLAY1"`)
   */
  friendly_name?: string | null;

  /**
   * Windows "monitor device name".
   *
   * Source: `MONITORINFOEXW.szDevice` via `GetMonitorInfoW()` in `WinUser.h`.
   *
   * Characteristics:
   *
   * - ⚠️ NOT stable across device disconnects/reconnects.
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
   * Human-friendly name of the adapter (typically the GPU).
   *
   * Source: `DISPLAY_DEVICEW.DeviceString` in `wingdi.h`.
   *
   * Example:
   *
   * - `"NVIDIA GeForce RTX 3050"`
   */
  adapter_friendly_name?: string | null;

  /**
   * Hardware identifier of the adapter (typically the GPU).
   *
   * Source: `DISPLAY_DEVICEW.DeviceID` in `wingdi.h`.
   *
   * Example:
   *
   * - `"PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1"`
   *
   * Characteristics:
   *
   * - ✅ Static physical hardware identifier.
   * - ✅ Stable and persistent across reboots and driver upgrades.
   * - ⚠️ Shared by all PORTS on a physical discrete GPU.
   * - ⚠️ Shared if you have multiple identical GPUs.
   *
   * For physical discrete GPUs, this value is the PCI ID. See:
   *
   * - https://learn.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-pci-devices
   * - https://pci-ids.ucw.cz/
   *
   * Format:
   *
   * ```
   * PCI \ VEN_10DE & DEV_2584 & SUBSYS_184610DE & REV_A1
   *       ┗━━━┳━━┛   ┗━━━┳━━┛   ┗━━━━━━┳━━━━━━┛   ┗━━┳━┛
   *        Vendor     Device       Subsystem     Revision
   * ```
   *
   * The above example is an NVIDIA GeForce RTX 3050 6GB PCIe graphics card:
   *
   * https://pcilookup.com/?ven=10DE&dev=2584&action=submit
   */
  adapter_hardware_id?: string | null;

  /**
   * Unique Plug-n-Play instance ID of the *individual GPU*, equal to
   * {@link adapter_hardware_id} + serial/location (PCI slot number).
   *
   * Source: `SetupDiGetDeviceInstanceIdW()` in `SetupAPI.h`.
   *
   * Example:
   *
   * - `"PCI\\VEN_10DE&DEV_0DF8&SUBSYS_083510DE&REV_A1\\4&2B1C6285&0&0010"`
   *
   * Characteristics:
   *
   * - ✅ Static physical hardware ID + serial + location (PCI slot number).
   * - ✅ Stable and persistent across reboots and driver upgrades.
   * - ⚠️ Shared by all PORTS on a physical discrete GPU.
   * - ✅ Unique per physical GPU instance.
   *
   * If your PC has two identical GPUs, each one will have its own unique
   * `adapter_instance_id` value.
   *
   * For a single GPU with multiple ports (e.g., one DisplayPort and one DVI),
   * all connected displays will have the same `adapter_instance_id` value.
   *
   * ### References
   *
   * From
   * [Windows Drivers > Device Instance ID](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/device-instance-ids):
   *
   * > A device instance ID is a system-supplied device identification string
   * > that uniquely identifies a device in the system. The Plug and Play (PnP)
   * > manager assigns a device instance ID to each device node (devnode) in a
   * > system's device tree.
   * >
   * > The creation of the device instance ID for a device uses the bus driver
   * > reported device ID value, instance ID value, and the UniqueID member of
   * > the DEVICE_CAPABILITIES structure as input in order to create the unique
   * > device instance ID for this device on the system.
   * >
   * > ```
   * > PCI\VEN_1000&DEV_0001&SUBSYS_00000000&REV_02\1&08
   * > ```
   */
  adapter_instance_id?: string | null;

  /**
   * Effectively a reformatted version of {@link adapter_instance_id}, plus a
   * static, hard-coded device interface class GUID for adapters
   * (`GUID_DEVINTERFACE_DISPLAY_ADAPTER`).
   *
   * Source: `DISPLAYCONFIG_ADAPTER_NAME.adapterDevicePath` in `wingdi.h`.
   *
   * Example:
   *
   * - `"\\\\?\\PCI#VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1#4&2b1c6285&0&0010#{5b45201d-f2f2-4f3b-85bb-30ff1f953599}"`
   *
   * Characteristics:
   *
   * - ✅ Stable and persistent across reboots.
   * - ❓ Unclear if persistent across driver upgrades.
   *
   * The GUID at the end is a fixed, Microsoft-defined class GUID (declared in
   * `Ntddvdeo.h`), and Windows appends it as part of the device-interface
   * symbolic link format when the display stack registers that interface.
   *
   * See:
   *
   * - [`GUID_DEVINTERFACE_DISPLAY_ADAPTER` constant (`Ntddvdeo.h`)](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/guid-devinterface-display-adapter)
   * - [`DISPLAYCONFIG_ADAPTER_NAME` structure (`wingdi.h`)](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_adapter_name)
   * - [`SetupDiGetDeviceInterfaceDetailA()` function (`setupapi.h`)](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetaila)
   * - [`IoRegisterDeviceInterface()` function (`wdm.h`)](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-ioregisterdeviceinterface)
   */
  adapter_device_path?: string | null;

  /**
   * Per-GPU-port registry key.
   *
   * Source: `DISPLAY_DEVICEW.DeviceKey` in `wingdi.h`.
   *
   * Examples:
   *
   * - `"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video\\{46D2BE53-1822-11F1-85AE-806E6F6E6963}\\0000"`
   * - `"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video\\{46D2BE53-1822-11F1-85AE-806E6F6E6963}\\0001"`
   *
   * Characteristics:
   *
   * - ❓ TODO(acdvorak): Describe uniqueness, persistence, and stability across
   *   reboots, device disconnects/reconnects, and port/dock changes.
   *
   * The GUID is a local, OS-generated adapter/video-stack identifier, likely
   * the adapter `VideoID` created by the video port / display driver machinery.
   *
   * The numeric suffix is an auto-incrementing "child adapter index" for the
   * physical port on the GPU (in the typical case).
   *
   * ### References
   *
   * According to
   * [WebRTC `win/screen_capture_utils.cc`](https://webrtc.googlesource.com/src/+/c0fd2e0/modules/desktop_capture/win/screen_capture_utils.cc?pli=1#184):
   *
   * > `DeviceKey` is documented as reserved, but it actually contains the
   * > registry key for the device and is unique for each monitor, while
   * > `DeviceID` is not.
   *
   * According to
   * [ReactOS Display Driver Loading](https://reactos.org/wiki/Techwiki:Win32k/display_driver_loading),
   * the format of this key is:
   *
   * > Device configuration key:
   * > `"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video\\<VideoId>\\0000"`
   * > where `<VideoId>` is a local UUID, created by `videoprt` the first time
   * > the device is started. The `VideoId` string is stored in the registry
   * > under the device's hardware key
   * > (`HKLM\System\CurrentControlSet\Enum\...`).
   */
  adapter_registry_key?: string | null;

  /**
   * Per-adapter identifier of the target display endpoint used by
   * `DisplayConfig` APIs to address/query a specific path target.
   *
   * Source: `DISPLAYCONFIG_PATH_INFO.targetInfo.id` in `wingdi.h`.
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
   * instance on that connection path. Useful for correlating to EDID retrieval.
   *
   * Source: `DISPLAYCONFIG_TARGET_DEVICE_NAME.monitorDevicePath` in `wingdi.h`.
   *
   * Examples:
   *
   * - `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
   * - `"\\\\?\\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
   *
   * Characteristics:
   *
   * - ❓ TODO(acdvorak): Describe stability/uniqueness
   * - ❓ TODO(acdvorak): What is the GUID?
   */
  monitor_device_path?: string | null;

  /**
   * ✅ SECONDARY STABLE ID (when available)
   *
   * Deterministic key derived from {@link monitor_device_path}.
   */
  monitor_path_key?: string | null;

  /**
   * Examples:
   *
   * - `"DISPLAY\\SAM73A5\\5&757FE5E&6&UID20737"`
   * - `"DISPLAY\\VIZ1009\\5&757FE5E&6&UID20739"`
   */
  monitor_instance_id?: string | null;

  /**
   * Examples:
   *
   * - `"{4d36e96e-e325-11ce-bfc1-08002be10318}\\0004"`
   * - `"{4d36e96e-e325-11ce-bfc1-08002be10318}\\0005"`
   *
   * The GUID `{4d36e96e-e325-11ce-bfc1-08002be10318}` is
   * `GUID_DEVCLASS_MONITOR`, which is the system-defined setup class for
   * monitors.
   *
   * The last 4 numeric digits (like `0004` or `0005`) represent a zero-padded
   * *instance identifier* (often called the *driver node index*) assigned
   * sequentially by the Windows Plug and Play (PnP) manager.
   *
   * Specifically, this 4-digit number acts as a pointer to the *Driver Key*
   * (also known as the *Software Key*) in the Windows Registry where the
   * operating system stores the driver and configuration parameters for that
   * exact monitor.
   *
   * When a monitor is connected, Windows looks at the Device Setup Class GUID
   * (the `{4d36e96e-e325-11ce-bfc1-08002be10318}` part, which dictates that
   * the device is a "Monitor") and assigns it the next available 4-digit number
   * starting from `0000`.
   *
   * This means the `0004` in the first example above maps directly to this
   * specific registry path, specified by {@link monitor_registry_key}:
   *
   * ```
   * HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e96e-e325-11ce-bfc1-08002be10318}\0004
   * ```
   *
   * If you navigate to that specific subkey in the Registry Editor, you will
   * find software-level properties for that monitor instance.
   */
  monitor_driver_key?: string | null;

  /**
   * Examples:
   *
   * - `"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e96e-e325-11ce-bfc1-08002be10318}\\0004"`
   * - `"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e96e-e325-11ce-bfc1-08002be10318}\\0005"`
   *
   * Common values stored here include:
   *
   * - **DriverDesc**: The friendly, human-readable name of the monitor
   *   (e.g., "Generic PnP Monitor", "Generic Non-PnP Monitor", or
   *   "Samsung SyncMaster").
   *
   * - **MatchingDeviceId**: The PnP hardware ID used to match the driver to the
   *   monitor (e.g., "*PNP09FF" or "MONITOR\Default_Monitor").
   *
   * - **ProviderName**: The author of the driver (usually "Microsoft" for
   *   standard Plug and Play monitors).
   *
   * - **EDID Overrides**: Any manual software overrides applied to the
   *   monitor's Extended Display Identification Data (EDID).
   */
  monitor_registry_key?: string | null;

  /**
   * Examples:
   *
   * - `"Generic PnP Monitor"`
   * - `"Generic Non-PnP Monitor"`
   *
   * TODO(acdvorak): Get this value from the registry (`DriverDesc`).
   * See {@link monitor_registry_key}.
   */
  monitor_string?: string | null;

  /**
   * Maybe EDID-derived?
   *
   * Example:
   *
   * - `"LAU8PSBP01000"` (Vizio TV)
   *
   * TODO(acdvorak): Figure out which Windows API returns this value.
   * NirSoft MultiMonitorTool knows how to get it.
   */
  monitor_serial_string?: string | null;

  /**
   * ✅ TERTIARY STABLE ID (when available)
   *
   * Deterministic monitor identity key based on EDID.
   *
   * Only emitted when manufacturer, product code, and serial are available.
   *
   * TODO(acdvorak): Append a hash of the full EDID bytes.
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
   * Typical values allowed by the Windows Display Settings UI are:
   *
   * `100 | 125 | 150 | 175 | 200 | 225 | 250 | 275 | 300 | ... | 500`
   *
   * Technically, the user can set any arbitrary value they want via registry
   * hacks.
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

  setup_api_devices: WinSetupApiDeviceCatalog[];
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

export interface WinSetupApiDeviceCatalog {
  adapters: WinSetupApiDevice[];
  monitors: WinSetupApiDevice[];
}

export interface WinSetupApiDevice {
  /**
   * ⚠️ Opaque device path. Use for case-insensitive string comparisons with
   * other APIs.
   *
   * Data source: `SP_DEVICE_INTERFACE_DETAIL_DATA_W.DevicePath`
   *
   * Usage:
   *
   * - ✅ Use it as a join key to correlate data from different APIs.
   * - ✅ ALWAYS use case-insensitive string comparisons.
   * - ❌ Do NOT assume it is always lowercase.
   * - ❌ Do NOT parse the value.
   *
   * According to Microsoft, there is no API contract that the string will
   * _always_ be lowercase:
   *
   * [Device identification strings](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/device-identification-strings):
   *
   * > Device identification strings **should not be parsed**. They are meant
   * > only for string comparisons and should be treated as **opaque strings**.
   *
   * [`SetupDiGetDeviceInterfaceDetailW() docs`](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetailw#remarks):
   *
   * > **Do not attempt to parse the device path symbolic name.**
   * >
   * > The device path can be reused across system starts.
   *
   * [`IoGetDeviceInterfaces()` docs](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-iogetdeviceinterfaces#remarks):
   *
   * > **The format of a symbolic link name is opaque; the caller should not
   * > attempt to parse a symbolic link name.**
   * >
   * > Symbolic links for device interface instances can be used across system
   * > boots.
   *
   * [`IoRegisterDeviceInterface()` docs](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-ioregisterdeviceinterface#parameters):
   *
   * > `SymbolicLinkName`: **kernel-mode path to the symbolic link** for an
   * > instance of the specified device interface class.
   * >
   * > **The caller must treat `SymbolicLinkName` as opaque** and **must not**
   * > disassemble it.
   *
   * Examples:
   *
   * - `"\\\\?\\display#sam73a5#5&757fe5e&7&uid20737#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
   * - `"\\\\?\\display#viz1009#5&757fe5e&7&uid20739#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
   * - `"\\\\?\\pci#ven_10de&dev_2584&subsys_184610de&rev_a1#4&2b1c6285&0&0010#{5b45201d-f2f2-4f3b-85bb-30ff1f953599}"`
   * - `"\\\\?\\root#basicdisplay#0000#{5b45201d-f2f2-4f3b-85bb-30ff1f953599}"`
   *
   */
  device_path_lowercase: string;

  /**
   * Data source: `SetupDiGetDeviceInstanceIdW()`
   *
   * Examples:
   *
   * - `"DISPLAY\\SAM73A5\\5&757FE5E&7&UID20737"`
   * - `"DISPLAY\\VIZ1009\\5&757FE5E&7&UID20739"`
   * - `"PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1\\4&2B1C6285&0&0010"`
   * - `"ROOT\\BASICDISPLAY\\0000"`
   */
  instance_id?: PnpInstanceId | null;

  /**
   * Examples:
   *
   * - `"Generic PnP Monitor"`
   * - `"Microsoft Basic Display Driver"`
   * - `"NVIDIA GeForce RTX 3050"`
   */
  device_desc?: string | null;

  /**
   * Examples:
   *
   * ```jsonc
   * [
   *   "PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1",
   *   "PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE",
   *   "PCI\\VEN_10DE&DEV_2584&CC_030000",
   *   "PCI\\VEN_10DE&DEV_2584&CC_0300"
   * ]
   * ```
   *
   * ```jsonc
   * [
   *   "ROOT\\BasicDisplay"
   * ]
   * ```
   *
   * ```jsonc
   * [
   *   "MONITOR\\SAM73A5"
   * ]
   * ```
   *
   * ```jsonc
   * [
   *   "MONITOR\\VIZ1009"
   * ]
   * ```
   */
  hardware_id?: PnpHardwareId[] | null;

  /**
   * Examples:
   *
   * ```jsonc
   * [
   *   "PCI\\VEN_10DE&DEV_2584&REV_A1",
   *   "PCI\\VEN_10DE&DEV_2584",
   *   "PCI\\VEN_10DE&CC_030000",
   *   "PCI\\VEN_10DE&CC_0300",
   *   "PCI\\VEN_10DE",
   *   "PCI\\CC_030000",
   *   "PCI\\CC_0300",
   * ]
   * ```
   *
   * ```jsonc
   * [
   *   "*PNP09FF"
   * ]
   * ```
   */
  compatible_ids?: PnpCompatibleId[] | null;

  /**
   * Examples:
   *
   * - `"nvlddmkm"`
   * - `"BasicDisplay"`
   * - `"monitor"`
   *
   * @string
   */
  service?: PnpServiceName | (string & {}) | null;

  /**
   * Examples:
   *
   * - `"Display"`
   * - `"Monitor"`
   * - `"System"`
   *
   * @string
   */
  class_name?: PnpClassName | (string & {}) | null;

  /**
   * Examples:
   *
   * - `"{4D36E968-E325-11CE-BFC1-08002BE10318}"` (Display)
   * - `"{4D36E96E-E325-11CE-BFC1-08002BE10318}"` (Monitor)
   * - `"{4D36E97D-E325-11CE-BFC1-08002BE10318}"` (System)
   *
   * @string
   */
  class_guid?: PnpClassGuid | (string & {}) | null;

  /**
   * Examples:
   *
   * - `"{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"`
   * - `"{4d36e96e-e325-11ce-bfc1-08002be10318}\\0004"`
   * - `"{4d36e96e-e325-11ce-bfc1-08002be10318}\\0005"`
   * - `"{4d36e97d-e325-11ce-bfc1-08002be10318}\\0051"`
   */
  driver?: PnpDriverGuidWithId | null;

  /**
   * Examples:
   *
   * - `0`
   *
   * @uint32
   */
  config_flags?: number | null;

  /**
   * Examples:
   *
   * - `"(Standard display types)"`
   * - `"(Standard monitor types)"`
   * - `"NVIDIA"`
   *
   * @string
   */
  mfg?: PnpMfgName | null;

  /**
   * Examples:
   *
   * - `"Generic Monitor (E390-B0)"`
   * - `"Generic Monitor (QCQ95S)"`
   */
  friendly_name?: PnpFriendlyName | null;

  /**
   * Examples:
   *
   * - `"PCI bus 5, device 0, function 0"`
   */
  location_information?: PnpLocationInformation | null;

  /**
   * Examples:
   *
   * - `"\\Device\\00000003"`
   * - `"\\Device\\0000006d"`
   * - `"\\Device\\0000006e"`
   * - `"\\Device\\NTPNP_PCI0024"`
   */
  physical_device_object_name?: PnpPhysicalDeviceObjectName | null;

  /**
   * Examples:
   *
   * - `0`
   * - `228`
   *
   * @uint32
   */
  capabilities?: number | null;

  /**
   * Example:
   *
   * - `2`
   *
   * @uint32
   */
  ui_number?: number | null;

  /**
   * Example:
   *
   * - `"{C6CA0D74-E43B-4ABD-A63A-4BD2AD319D60}"`
   * - `"{C8EBDFB0-B510-11D0-80E5-00A0C92542E3}"`
   */
  bus_type_guid?: PnpGuidFormat | null;

  /**
   * Example:
   *
   * - `5`
   * - `15`
   *
   * @uint32
   */
  legacy_bus_type?: number | null;

  /**
   * Example:
   *
   * - `0`
   * - `5`
   *
   * @uint32
   */
  bus_number?: number | null;

  /**
   * Example:
   *
   * - `"DISPLAY"`
   * - `"PCI"`
   * - `"ROOT"`
   *
   * @string
   */
  enumerator_name?: PnpEnumeratorName | (string & {}) | null;

  /**
   * @uint32
   */
  dev_type?: number | null;

  /**
   * @uint32
   */
  characteristics?: number | null;

  /**
   * Example:
   *
   * - `0`
   * - `273`
   * - `275`
   *
   * @uint32
   */
  address?: number | null;

  ui_number_desc_format?: string | null;

  /**
   * Examples:
   *
   * ```jsonc
   * [
   *   "PCIROOT(0)#PCI(0200)#PCI(0000)",
   *   "ACPI(_SB_)#ACPI(PCI0)#ACPI(NPE2)#ACPI(SLT2)",
   * ]
   * ```
   */
  location_paths?: PnpLocationPath[] | null;

  /**
   * Examples:
   *
   * - `"{00000000-0000-0000-FFFF-FFFFFFFFFFFF}"`
   * - `"{840576F3-7055-5A87-9859-A63E0FA9F3DA}"`
   * - `"{DA0631A1-C6DF-5655-BEA1-9110DBA37845}"`
   */
  base_container_id?: PnpGuidFormat | null;
}

/**
 * Examples:
 *
 * ```jsonc
 * [
 *   "PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1",
 *   "PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE",
 *   "PCI\\VEN_10DE&DEV_2584&CC_030000",
 *   "PCI\\VEN_10DE&DEV_2584&CC_0300"
 * ]
 * ```
 */
export type PnpPciId =
  | `PCI\\VEN_${string}&DEV_${string}&SUBSYS_${string}&REV_${string}`
  | `PCI\\VEN_${string}&DEV_${string}&SUBSYS_${string}`
  | `PCI\\VEN_${string}&DEV_${string}&CC_${string}`;

/**
 * Examples:
 *
 * ```jsonc
 * [
 *   "PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1",
 *   "PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE",
 *   "PCI\\VEN_10DE&DEV_2584&CC_030000",
 *   "PCI\\VEN_10DE&DEV_2584&CC_0300"
 * ]
 * ```
 *
 * ```jsonc
 * [
 *   "ROOT\\BasicDisplay"
 * ]
 * ```
 *
 * ```jsonc
 * [
 *   "MONITOR\\SAM73A5"
 * ]
 * ```
 *
 * ```jsonc
 * [
 *   "MONITOR\\VIZ1009"
 * ]
 * ```
 */
export type PnpHardwareId = PnpPciId | `ROOT\\${string}` | `MONITOR\\${string}`;

/**
 * Examples:
 *
 * - `"DISPLAY\\SAM73A5\\5&757FE5E&7&UID20737"`
 * - `"DISPLAY\\VIZ1009\\5&757FE5E&7&UID20739"`
 * - `"PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1\\4&2B1C6285&0&0010"`
 * - `"ROOT\\BASICDISPLAY\\0000"`
 */
export type PnpInstanceId =
  | `DISPLAY\\${string}\\${number}&${string}&${number}&UID${number}}`
  | `PCI\\VEN_${string}&DEV_${string}&SUBSYS_${string}&REV_${string}\\${number}&${string}&${number}&${string}`
  | `ROOT\\${string}\\${string}`;

/**
 * Examples:
 *
 * ```jsonc
 * [
 *   "PCI\\VEN_10DE&DEV_2584&REV_A1",
 *   "PCI\\VEN_10DE&DEV_2584",
 *   "PCI\\VEN_10DE&CC_030000",
 *   "PCI\\VEN_10DE&CC_0300",
 *   "PCI\\VEN_10DE",
 *   "PCI\\CC_030000",
 *   "PCI\\CC_0300",
 * ]
 * ```
 *
 * ```jsonc
 * [
 *   "*PNP09FF"
 * ]
 * ```
 */
export type PnpCompatibleId = PnpPciId | `PCI\\CC_${string}` | `*PNP${string}`;

export type PnpClassName = 'Display' | 'Monitor' | 'System';

export type PnpGuidFormat =
  `{${string}-${string}-${string}-${string}-${string}}`;

export type PnpDisplayGuid = '{4D36E968-E325-11CE-BFC1-08002BE10318}';
export type PnpMonitorGuid = '{4D36E96E-E325-11CE-BFC1-08002BE10318}';
export type PnpSystemGuid = '{4D36E97D-E325-11CE-BFC1-08002BE10318}';

export type PnpClassGuid = PnpDisplayGuid | PnpMonitorGuid | PnpSystemGuid;

export type PnpDriverGuidWithId = `${Lowercase<PnpClassGuid>}\\${string}`;

export type PnpServiceName = 'BasicDisplay' | 'monitor';

export type PnpMfgName =
  | '(Standard display types)'
  | '(Standard monitor types)'
  | 'NVIDIA'
  | (string & {});

export type PnpFriendlyName = `Generic Monitor (${string})`;

export type PnpLocationInformation =
  `PCI bus ${number}, device ${number}, function ${number}`;

export type PnpPhysicalDeviceObjectName = `\\Device\\${string}`;

export type PnpEnumeratorName = 'DISPLAY' | 'PCI' | 'ROOT';

export type PnpLocationPath =
  | `PCIROOT(${string})#PCI(${string})#PCI(${string})`
  | `ACPI(_SB_)#ACPI(PCI${number})#ACPI(NPE${number})#ACPI(SLT${number})`;

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
