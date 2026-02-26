#include "DxgiOutputDevice.h"

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

void AppendDxgiOutputDevices(
    ComPtr<IDXGIAdapter> pIDXGIAdapter,
    std::map<ShortLivedIdentifier, dxgi::DxgiOutputDevice>& devices) {
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

    // Represents an adapter output (such as a monitor).
    // The IDXGIOutput6 interface exposes methods to provide specific
    // monitor capabilities.
    ComPtr<IDXGIOutput6> output6;
    if (!SUCCEEDED(pIDXGIOutput.As(&output6)) || !output6) {
      continue;
    }

    // Describes an output or physical connection between the adapter
    // (video card) and a device, including additional information about
    // color capabilities and connection type.
    DXGI_OUTPUT_DESC1 desc;
    if (!SUCCEEDED(output6->GetDesc1(&desc))) {
      continue;
    }

    ShortLivedIdentifier deviceNameUtf8 = WideToUtf8(desc.DeviceName);

    dxgi::DxgiOutputDevice& device = devices[deviceNameUtf8];

    device.device_name = deviceNameUtf8;

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
    device.is_attached_to_desktop = desc.AttachedToDesktop;

    device.desktop_coordinates = desc.DesktopCoordinates;

    device.rotation_type = desc.Rotation;

    device.color_space = desc.ColorSpace;
    device.bits_per_color = desc.BitsPerColor;

    if (desc.MinLuminance > 0 || desc.MaxLuminance > 0 ||
        desc.MaxFullFrameLuminance > 0) {
      device.min_luminance = desc.MinLuminance;
      device.max_luminance = desc.MaxLuminance;
      device.max_full_frame_luminance = desc.MaxFullFrameLuminance;
    }
  }
}

}  // namespace

namespace dxgi {

std::map<ShortLivedIdentifier, dxgi::DxgiOutputDevice> GetDxgiOutputDevices() {
  std::map<ShortLivedIdentifier, dxgi::DxgiOutputDevice> devices;

  // DXGI APIs will crash or hang if we try to call them in a non-interactive
  // session.
  if (!sys::HasInteractiveDesktop()) {
    return devices;
  }

  ComPtr<IDXGIFactory> pIDXGIFactory;

  auto ppFactory = reinterpret_cast<void**>(pIDXGIFactory.GetAddressOf());
  auto isFactoryCreated =
      SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), ppFactory));

  if (!isFactoryCreated || !pIDXGIFactory) {
    return devices;
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

    AppendDxgiOutputDevices(pIDXGIAdapter, devices);
  }

  return devices;
}

}  // namespace dxgi
