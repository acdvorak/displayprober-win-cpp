# DisplayProber for Windows (DP4Win) 🛰️

CLI that outputs a list of all connected displays (monitors/TVs) as
[JSON](./src/ts/schemas/displayprober-win-cpp.schema.json).

Supports Windows 7 SP1 and newer, both 32-bit and 64-bit. \
Tested in Windows 7 SP1, Windows 10, and Windows 11.

This tool returns:

- Friendly display name
- Resolution, working area, rotation, and refresh rate
- Primary vs. extended
- Physical connector type (HDMI, DisplayPort, DVI, VGA, internal, etc.)
- Unique-ish identifier (not stable across reboots, unfortunately)
- Basic EDID properties like manufacturer name, manufacture week/year, physical
  screen dimensions (width and height in MM), serial number, and product code
- Raw EDID bytes

Detects if the following features are _supported_ and/or _actively enabled_:

- HDR10
- Wide/advanced color
- Color space and bit depth
- Min/max luminance
- DPI scaling
- Remote Desktop (RDP)
- Running inside a virtual machine client

_NOTE: For feature detection to work, every device in the chain must support
that feature: OS, GPU, monitor/TV, **and** HDMI/DisplayPort cables._

# Usage

```ps1
DisplayProber.exe
```

# Example output

## Windows 10 PC

```jsonc
{
  "displays": [
    // Primary display: Samsung S95C TV
    {
      "friendly_name": "QCQ95S",
      "short_lived_identifier": "\\\\.\\DISPLAY1",
      "monitor_device_path": "\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}",
      "is_primary": true,
      "physical_connector_type": "hdmi",
      "rotation_deg": 0,
      "dpi_scaling_percent": 150,
      "bounds": {
        "x": 0,
        "y": 0,
        "width": 1900,
        "height": 1058,
        "left": 0,
        "right": 1900,
        "top": 0,
        "bottom": 1058,
      },
      "working_area": {
        "x": 0,
        "y": 0,
        "width": 1900,
        "height": 998,
        "left": 0,
        "right": 1900,
        "top": 0,
        "bottom": 998,
      },
      "scan_line_ordering": "progressive",
      "refresh_rate_hz": 60.0,
      "refresh_rate_numerator": 60000,
      "refresh_rate_denominator": 1000,
      "standard_color_info": {
        "bits_per_channel": 8,
        "color_encoding": "rgb",
        "dxgi_color_space": "rgb_full_g22_none_p709",
        "is_hdr_supported": false,
        "is_hdr_enabled": false,
        "min_luminance": 0.009999999776482582,
        "max_luminance": 1499.0,
        "max_full_frame_luminance": 799.0,
      },
      "advanced_color_info": {
        "is_advanced_color_active": false,
        "is_advanced_color_enabled": false,
        "is_advanced_color_force_disabled": false,
        "is_advanced_color_limited_by_policy": false,
        "is_advanced_color_supported": false,
        "is_high_dynamic_range_supported": false,
        "is_high_dynamic_range_user_enabled": false,
        "is_wide_color_enforced": false,
        "is_wide_color_supported": false,
        "is_wide_color_user_enabled": false,
      },
      "edid_info": {
        "manufacturer_vid": "SAM",
        "user_friendly_name": "QCQ95S",
        "max_horizontal_image_size_mm": 1420.0,
        "max_vertical_image_size_mm": 800.0,
        "product_code_id": 7346,
        "serial_number_id": 16780800,
        "week_of_manufacture": 1,
        "year_of_manufacture": 2023,
        "edid_bytes_base64": "AP///////wBMLUZzAA4AAQEhAQOAjlB4CvQRskpBsyY...=",
      },
    },

    // Secondary display: Dell ST2320L monitor
    {
      "friendly_name": "DELL ST2320L",
      "short_lived_identifier": "\\\\.\\DISPLAY2",
      "monitor_device_path": "\\\\?\\DISPLAY#DELF023#5&21e6c3...",
      "is_primary": false,
      "physical_connector_type": "dvi",
      "rotation_deg": 0,
      "dpi_scaling_percent": 150,
      "...": "...",
    },
  ],
}
```

# Feature detection

- Monitor/display device name(s)
- Resolution (`W` × `H`)
- Refresh rate (Hz)
- Bit depth per channel (8, 10, 12, 14, 16)
- DPI scaling (100%, 125%, 150%, etc.)
- Physical connector type
  - VGA
  - DVI
  - HDMI
  - DisplayPort
  - Internal
- Color support:
  - HDR10 supported and active (enabled/disabled)
  - Advanced color supported and active (enabled/disabled)
  - Wide color supported and active (enabled/disabled)
  - Color space: RGB, YCbCr444, YCbCr422, YCbCr420

# Windows version support

- Windows 7 SP1+:
  - Monitor/display names
  - Resolution
  - Refresh rate
  - Physical connector type (HDMI/DVI/VGA/etc.)

- Windows 8.1+:
  - Per-monitor DPI scaling (100%, 125%, 150%, etc.)

- Windows 10 1607+:
  - Per-thread DPI scaling (100%, 125%, 150%, etc.)
  - HDR10 supported and enabled/disabled
  - Advanced color supported and enabled/disabled

- Windows 11 24H2+:
  - Wide color supported and enabled/disabled
  - Active color mode (SDR/WCG/HDR)

# Implementation details

## So you want to enumerate displays on Windows...

...and you want stable, persistent identifiers to uniquely track each monitor/TV
instance across reboots.

Welcome, my friend, to a world of pain.

![One does not simply "get a list of displays" on Windows](./boromir.jpg)

For your journey into the abyss, you will need:

1. An elf, a dwarf, and a hobbit
2. A stout constitution
3. A licensed professional therapist

To truly capture everything there is to know about all connected displays, you
must use a combination of four different Windows API families and
correlate/aggregate the results.

## 1. User32 multi-monitor APIs (`HMONITOR`)

What they are:

> Classic Win32 display monitor enumeration APIs (`HMONITOR`).

What they do:

> Enumerate "display monitors" in the Windows desktop/virtual-screen coordinate
> space, including pseudo-monitors (e.g. mirroring drivers).

Primary functions:

- `EnumDisplayMonitors()`
  - Returns a list of `HMONITOR` handles
- `GetMonitorInfoW()`
  - Fills `MONITORINFO` / `MONITORINFOEX` structures (bounds + work area +
    optional device name)

Pseudo-code (simplified for clarity):

```cpp
EnumDisplayMonitors(..., callback = (HMONITOR h) => {
  GetMonitorInfoW(h, MONITORINFOEX* info);
  // info->rcMonitor, info->rcWork, info->szDevice
})
```

## 2. Display Configuration APIs (`QueryDisplayConfig`)

What they are:

> A topology-level view of the current display configuration: "paths" + "modes".

What they give you:

- `QueryDisplayConfig()`: topology graph (sources/targets, active paths, mode.
  sets).
- `DisplayConfigGetDeviceInfo()`: per-target details like EDID-derived
  identifiers (not raw EDID bytes), friendly names, and device paths.

Primary functions:

- `QueryDisplayConfig()`
  - Returns `DISPLAYCONFIG_PATH_INFO` + `DISPLAYCONFIG_MODE_INFO`
- `DisplayConfigGetDeviceInfo()`
  - Packet-based details (e.g. target name, preferred mode, advanced color info)
  - Note: can fail when you do not have access to the current console session
    (common in services / some remote contexts)

Pseudo-code (simplified for clarity):

```cpp
QueryDisplayConfig(..., out paths[], out modes[]);

for (auto path in paths[]) {
  // identify adapterId + targetId from the path
  DisplayConfigGetDeviceInfo(
    GET_TARGET_NAME, adapterId, targetId, out targetName
  );
  DisplayConfigGetDeviceInfo(
    GET_ADVANCED_COLOR_INFO, adapterId, targetId, out colorInfo
  );
}
```

## 3. DXGI output enumeration APIs (`IDXGIOutput`)

What they are:

> Adapter/output model (DirectX Graphics Infrastructure).

What they give you:

> Advanced color and luminance characteristics (via `DXGI_OUTPUT_DESC1`).

Primary functions:

- `CreateDXGIFactory()` (or `CreateDXGIFactory1()`)
- `IDXGIFactory::EnumAdapters()` (or `EnumAdapters1()`)
- `IDXGIAdapter::EnumOutputs()`
- `IDXGIOutput6::GetDesc1()`

Pseudo-code (simplified for clarity):

```cpp
factory = CreateDXGIFactory(...);

for (auto adapter in factory.EnumAdapters()) {
  for (auto output in adapter.EnumOutputs()) {
    output6 = (IDXGIOutput6)output
    output6.GetDesc1(out DXGI_OUTPUT_DESC1 desc)
  }
}
```

## 4. SetupAPI and PnP device properties

What they are:

> PnP device tree enumeration + device properties.

What they give you:

- Raw EDID bytes (commonly via the monitor devnode's registry key; often a value
  named `EDID`)
- Device instance IDs and Hardware IDs (devnode-dependent: GPU vs monitor
  differ)
- Location paths (often the most useful for physical/topology correlation; PCI
  topology strings are common for GPUs)
- Container ID (GUID) (groups devnodes into a physical device container)

Primary functions:

- `SetupDiGetClassDevsW()`
- `SetupDiEnumDeviceInfo()` (or `SetupDiEnumDeviceInterfaces()`)
- `SetupDiGetDeviceInstanceIdW()`
- `SetupDiGetDeviceRegistryPropertyW()` (legacy) or
  `SetupDiGetDevicePropertyW()` (recommended for modern device properties)
- `SetupDiOpenDevRegKey()` + `RegQueryValueExW()` (commonly used to read `EDID`)

Pseudo-code (simplified for clarity):

```cpp
h = SetupDiGetClassDevsW(MONITOR_CLASS_GUID, ...);

for (auto dev in SetupDiEnumDeviceInfo(h)) {
  instId      = SetupDiGetDeviceInstanceIdW(h, dev);
  locPaths    = SetupDiGetDevicePropertyW(h, dev, DEVPKEY_Device_LocationPaths);
  containerId = SetupDiGetDevicePropertyW(h, dev, DEVPKEY_Device_ContainerId);
  regKey      = SetupDiOpenDevRegKey(h, dev, ...);
  edid        = RegQueryValueExW(regKey, "EDID");
}
```

## Stable identifiers

TODO

# PowerShell equivalents

In Windows PowerShell v5.1 and above, you can run the following commands:

```ps1
# Windows Forms screen enumeration.
# Returns a default placeholder display named "WinDisc", 1024x768, when called
# over SSH.
Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.Screen]::AllScreens |
  ConvertTo-Json

# GPU(s).
# Works even over SSH.
Get-CimInstance Win32_VideoController |
  ConvertTo-Json

# Windows "PnP" monitors.
# Works even over SSH.
Get-CimInstance Win32_PnPEntity |
  Where-Object { $_.PNPClass -in @('Monitor','Display') } |
  ConvertTo-Json

# Parsed EDID "Basic Display Parameters" block.
# Works even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorBasicDisplayParams |
  ConvertTo-Json

# Physical connector type (HDMI, DisplayPort, DVI, VGA, etc.).
# Works even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorConnectionParams |
  ConvertTo-Json

# Parsed EDID "Video Input Definition" block.
# Works even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorDescriptorMethods |
  ConvertTo-Json

# Parsed EDID "Vendor/Product Identification" block.
# Works even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorID |
  ConvertTo-Json
```

# Related tools

Native binaries:

- [NirSoft DumpEDID](https://www.nirsoft.net/utils/dump_edid.html)
  - Reads and parses EDID data
- [NirSoft MonitorInfoView](https://www.nirsoft.net/utils/monitor_info_view.html)
  - Reads and parses EDID data
- [NirSoft MultiMonitorTool](https://www.nirsoft.net/utils/multi_monitor_tool.html)
  - Prints most of the basic facts that Windows knows about your monitors:
    resolution, refresh rate, DPI scaling, rotation, position
- [NirSoft ControlMyMonitor](https://www.nirsoft.net/utils/control_my_monitor.html)
  - Prints a few basic details about your monitors, and lets you query/control
    their brightness, contrast, etc. via DDC/CI
- [`edid-decode`](https://git.linuxtv.org/v4l-utils.git/tree/utils/edid-decode)
  - Reference-quality EDID and DisplayID parser
  - Nominally cross-platform, but harder to compile on Windows/macOS
  - [Windows port of `edid-decode`](https://github.com/a1ive/edid-decode)

```ps1
DumpEDID.exe -a > path/to/output.txt

MonitorInfoView.exe /HideInactiveMonitors 1 /sxml path/to/output.xml

MultiMonitorTool.exe /HideInactiveMonitors 1 /sxml path/to/output.xml

ControlMyMonitor.exe /smonitors path/to/output.txt

edid-decode.exe /MONITOR0
edid-decode.exe path/to/edid.bin
edid-decode.exe /MONITOR0 path/to/edid.bin
```

# Related projects

- [AutoActions: `HDRController.cpp`](https://github.com/Codectory/AutoActions/blob/main/Source/HDRController/HDRController/HDRController.cpp)
- [HDR Tray: `HDR.cpp`](https://github.com/res2k/HDRTray/blob/main/common/HDR.cpp)
- [WinUIEx: `MonitorInfo.cs`](https://github.com/dotMorten/WinUIEx/blob/main/src/WinUIEx/MonitorInfo.cs)
- `libdisplay-info`:
  - EDID and DisplayID parsing library written in C
  - [freedesktop.org source code](https://gitlab.freedesktop.org/emersion/libdisplay-info)
    - [API documentation](https://emersion.pages.freedesktop.org/libdisplay-info/)
    - [`edid-decode` subproject branch](https://gitlab.freedesktop.org/emersion/libdisplay-info/-/tree/edid-decode-subproject)
    - [`wasm` branch](https://gitlab.freedesktop.org/emersion/libdisplay-info/-/tree/wasm)
  - [LineageOS fork](https://github.com/LineageOS/android_external_libdisplay-info-upstream)
  - [Chromium mirror](https://chromium.googlesource.com/external/gitlab.freedesktop.org/emersion/libdisplay-info/)
  - [GitHub mirror](https://github.com/gjasny/v4l-utils)

# Development

## Prerequisites

- Windows 10 or newer
- Windows PowerShell v5.1 (built in to all versions of Windows) or newer

Run:

```ps1
.\deps.ps1
```

This will install all necessary dependencies:

- [`winget`](https://aka.ms/winget)
- [CMake](https://cmake.org/)
- [VS 2022 Build Tools installer](https://learn.microsoft.com/en-us/visualstudio/releases/2022/release-history#fixed-version-bootstrappers)
  - MSVC v143 - VS 2022 C++ x64/86 build tools
  - C++ CMake tools for Windows
  - Windows 11 SDK v10.26100
  - MSBuild

## Building

```ps1
.\build.ps1 [-Debug|-Release] [-x64|-x86|-Both]
```

## Running

```ps1
.\run.ps1
```
