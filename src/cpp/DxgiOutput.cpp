// DirectX Graphics Infrastructure.
//
// Provides advanced color and luminance characteristics.

#include "DxgiOutput.h"

#include <dxgi.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <map>
#include <string>

#include "CommonTypes.h"
#include "StringUtils.h"
#include "SysUtils.h"

using Microsoft::WRL::ComPtr;

namespace {

void AppendDxgiOutputInfos(
    ComPtr<IDXGIAdapter> pIDXGIAdapter,
    std::map<ShortLivedIdentifier, dxgi::DxgiOutputInfo>& dxgi_output_infos) {
  ComPtr<IDXGIOutput> pIDXGIOutput;

  for (UINT output = 0;; ++output) {
    const HRESULT outputHr = pIDXGIAdapter->EnumOutputs(
        output, pIDXGIOutput.ReleaseAndGetAddressOf());

    if (outputHr == DXGI_ERROR_NOT_FOUND) {
      break;
    }

    if (FAILED(outputHr) || !pIDXGIOutput) {
      continue;
    }

    // Describes an output or physical connection between the adapter
    // (video card) and a device, including additional information about
    // color capabilities and connection type.
    //
    // Available in Windows Vista RTM and later.
    DXGI_OUTPUT_DESC desc0;
    if (!SUCCEEDED(pIDXGIOutput->GetDesc(&desc0))) {
      continue;
    }

    ShortLivedIdentifier short_lived_identifier = WideToUtf8(desc0.DeviceName);
    dxgi::DxgiOutputInfo& dxgi = dxgi_output_infos[short_lived_identifier];

    dxgi.short_lived_identifier = short_lived_identifier;

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
    dxgi.is_attached_to_desktop = desc0.AttachedToDesktop;

    dxgi.process_local_monitor_handle_ptr =
        reinterpret_cast<std::uintptr_t>(desc0.Monitor);

    dxgi.desktop_coordinates = desc0.DesktopCoordinates;
    dxgi.rotation_type = desc0.Rotation;

    // Represents an adapter output (such as a monitor).
    // The `IDXGIOutput6` interface exposes methods to provide specific
    // monitor capabilities.
    //
    // Windows 10 and newer.
    //
    // On Windows 8.1 and earlier, where `IDXGIOutput6` is not implemented,
    // `pIDXGIOutput.As(&output6)` will return `E_NOINTERFACE` (a failed
    // `HRESULT`); it will not crash.
    ComPtr<IDXGIOutput6> output6;
    if (!SUCCEEDED(pIDXGIOutput.As(&output6)) || !output6) {
      continue;
    }

    // Describes an output or physical connection between the adapter
    // (video card) and a device, including additional information about
    // color capabilities and connection type.
    DXGI_OUTPUT_DESC1 desc1;
    if (!SUCCEEDED(output6->GetDesc1(&desc1))) {
      continue;
    }

    dxgi.color_space = desc1.ColorSpace;
    dxgi.bits_per_channel = desc1.BitsPerColor;

    if (desc1.MinLuminance > 0 || desc1.MaxLuminance > 0 ||
        desc1.MaxFullFrameLuminance > 0) {
      dxgi.min_luminance_nits = desc1.MinLuminance;
      dxgi.max_luminance_nits = desc1.MaxLuminance;
      dxgi.max_full_frame_luminance_nits = desc1.MaxFullFrameLuminance;
    }
  }
}

}  // namespace

namespace dxgi {

std::map<ShortLivedIdentifier, dxgi::DxgiOutputInfo> GetDxgiOutputInfos() {
  std::map<ShortLivedIdentifier, dxgi::DxgiOutputInfo> dxgi_output_infos;

  // DXGI APIs will crash or hang if we try to call them in a non-interactive
  // session.
  if (!sys::HasInteractiveDesktop()) {
    return dxgi_output_infos;
  }

  ComPtr<IDXGIFactory> pIDXGIFactory;

  auto ppFactory = reinterpret_cast<void**>(pIDXGIFactory.GetAddressOf());
  auto isFactoryCreated =
      SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), ppFactory));

  if (!isFactoryCreated || !pIDXGIFactory) {
    return dxgi_output_infos;
  }

  ComPtr<IDXGIAdapter> pIDXGIAdapter;

  for (UINT adapter = 0;; ++adapter) {
    const HRESULT adapterHr = pIDXGIFactory->EnumAdapters(
        adapter, pIDXGIAdapter.ReleaseAndGetAddressOf());

    if (adapterHr == DXGI_ERROR_NOT_FOUND) {
      break;
    }

    if (FAILED(adapterHr) || !pIDXGIAdapter) {
      continue;
    }

    AppendDxgiOutputInfos(pIDXGIAdapter, dxgi_output_infos);
  }

  return dxgi_output_infos;
}

}  // namespace dxgi
