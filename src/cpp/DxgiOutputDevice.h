#pragma once

#include <dxgi.h>

#include <map>
#include <optional>
#include <string>

#include "CommonTypes.h"

namespace dxgi {

// DirectX Graphics Infrastructure (DXGI) is a Microsoft Windows component
// (Vista+) that manages low-level graphics tasks, such as enumerating adapters
// and display modes, and presenting rendered frames to a display.
//
// It acts as a bridge between user-mode graphics APIs (Direct3D 10/11/12) and
// the Windows Display Driver Model (WDDM), handling hardware interfacing,
// buffer management, and resource sharing, including screen capturing via the
// Desktop Duplication API.
struct DxgiOutputDevice {
  // Windows "monitor device name" from `MONITORINFOEX.szDevice`.
  //
  // ⚠️ NOT stable across device disconnects/reconnects.
  //
  // Examples:
  //
  // ```
  // "\\\\.\\DISPLAY1"   (multi-monitor)
  // "\\\\.\\DISPLAY2"   (multi-monitor)
  // "\\\\.\\DISPLAY129" (Remote Desktop)
  // "DISPLAY"           (single-monitor)
  // "WinDisc"           (SSH console)
  // ```
  ShortLivedIdentifier device_name;

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
  //     "Show only on 1" (or you’ve "Disconnect this display" for the other).
  //     That other output can still exist, but it is not attached, so false.
  bool is_attached_to_desktop = false;

  std::optional<RECT> desktop_coordinates;
  std::optional<DXGI_MODE_ROTATION> rotation_type;

  std::optional<DXGI_COLOR_SPACE_TYPE> color_space;
  std::optional<UINT> bits_per_color;
  std::optional<FLOAT> min_luminance;
  std::optional<FLOAT> max_luminance;
  std::optional<FLOAT> max_full_frame_luminance;
};

std::map<ShortLivedIdentifier, DxgiOutputDevice> GetDxgiOutputDevices();

}  // namespace dxgi
