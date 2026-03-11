#pragma once

#include <dxgi.h>

#include <cstdint>
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
  // Windows "monitor device name".
  //
  // Corresponds to `BasicMonitorInfo.short_lived_identifier`.
  //
  // Value comes from `DXGI_OUTPUT_DESC1.DeviceName[32]`,
  // populated by `(ComPtr<IDXGIOutput6> output6)->GetDesc1(&desc)`.
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
  ShortLivedIdentifier short_lived_identifier;

  // Raw `HMONITOR` handle (pointer value) from `DXGI_OUTPUT_DESC1.Monitor`,
  // populated by `(ComPtr<IDXGIOutput6> output6)->GetDesc1(&desc)`.
  //
  // Corresponds to `BasicMonitorInfo.process_local_monitor_handle_ptr`.
  //
  // Value is process-local and NOT stable across topology changes
  // (device connects/disconnects).
  //
  // Example: `17749131`
  std::uintptr_t process_local_monitor_handle_ptr = 0;

  // Indicates whether the output device is connected, enabled, and active
  // (i.e., currently being used).
  //
  // Value comes from `DXGI_OUTPUT_DESC1.AttachedToDesktop`
  // populated by `(ComPtr<IDXGIOutput6> output6)->GetDesc1(&desc)`.
  //
  // A value of `false` means "this output exists, but Windows is not currently
  // using it as part of the active desktop topology (the active VidPN)".
  //
  // If Windows Display Settings would show it as "not connected" or
  // "not in use" (even if the cable is plugged in), DXGI can still enumerate
  // an output object for it, and `AttachedToDesktop` is the bit that tells you
  // whether it is actually in the current desktop.
  bool is_attached_to_desktop = false;

  std::optional<RECT> desktop_coordinates;
  std::optional<DXGI_MODE_ROTATION> rotation_type;

  std::optional<DXGI_COLOR_SPACE_TYPE> color_space;

  // Bits per color channel.
  //
  // Typical real-world values:
  //
  // - `6`: Rare, but can appear when the active wire format is effectively
  //   6 bpc (or 6 bpc + dithering).
  // - `8`: Most SDR desktop setups (standard 8 bpc output).
  // - `10`: Very common when Windows "Use HDR" / advanced color is active, or
  //   when the active output format is 10 bpc.
  // - `12`: Less common, but shows up on some HDR-capable pipelines
  //   (often depends on GPU, link bandwidth, and chosen output format).
  //
  // The API field is just a UINT describing the active wire format, so oddball
  // values are not forbidden.
  //
  // NOTES:
  //
  // - This is "bits per color channel for the active wire format" on that
  //   output, not "panel native bit depth". So an "8-bit + FRC" panel can still
  //   report 10 if the link is running 10 bpc.
  //
  // - Seeing 8 bpc does not mean "not HDR". There are reports of HDR
  //   colorspaces while BitsPerColor is 8, depending on
  //   driver/cable/mode behavior.
  std::optional<UINT> bits_per_channel;

  std::optional<FLOAT> min_luminance_nits;
  std::optional<FLOAT> max_luminance_nits;
  std::optional<FLOAT> max_full_frame_luminance_nits;
};

std::map<ShortLivedIdentifier, DxgiOutputDevice> GetDxgiOutputDevices();

}  // namespace dxgi
