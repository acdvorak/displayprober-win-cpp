#include "EdidBytes.h"

// TODO(acdvorak): Can this be removed?
// Source:
// https://github.com/Aleksoid1978/DisplayInfo/blob/ee114bc24/DisplayConfig/DisplayConfig.h#L23-L26
#ifndef _MSC_VER
#define WINVER 0x0605
#define NTDDI_VERSION NTDDI_WINBLUE
#endif

// This header needs to be imported first.
#include <Windows.h>

// Keep other .h headers separate from Windows.h to prevent auto-sorting.
#include <SetupAPI.h>
#include <initguid.h>
#include <ntddvdeo.h>

#include <cstdint>
#include <vector>

#include "CommonTypes.h"
#include "StringUtils.h"

namespace {

class ScopedDevInfoSet {
 public:
  explicit ScopedDevInfoSet(HDEVINFO handle) : handle_(handle) {}

  ~ScopedDevInfoSet() {
    if (handle_ != INVALID_HANDLE_VALUE) {
      SetupDiDestroyDeviceInfoList(handle_);
    }
  }

  HDEVINFO get() const { return handle_; }

 private:
  HDEVINFO handle_ = INVALID_HANDLE_VALUE;
};

class ScopedRegKey {
 public:
  explicit ScopedRegKey(HKEY key) : key_(key) {}

  ~ScopedRegKey() {
    if (key_ != nullptr && key_ != INVALID_HANDLE_VALUE) {
      RegCloseKey(key_);
    }
  }

  HKEY get() const { return key_; }

 private:
  HKEY key_ = nullptr;
};

std::optional<Bytes> ReadEdidBytes(HDEVINFO dev_info_set,
                                   SP_DEVINFO_DATA dev_info_data) {
  const HKEY registry_key = SetupDiOpenDevRegKey(
      dev_info_set, &dev_info_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
  if (registry_key == nullptr || registry_key == INVALID_HANDLE_VALUE) {
    return std::nullopt;
  }

  ScopedRegKey scoped_key(registry_key);

  DWORD type = 0;
  DWORD size = 0;
  const LONG query_size_result = RegQueryValueExW(
      scoped_key.get(), L"EDID", nullptr, &type, nullptr, &size);
  if (query_size_result != ERROR_SUCCESS || type != REG_BINARY || size == 0) {
    return std::nullopt;
  }

  Bytes bytes(size);
  DWORD read_size = size;
  type = 0;
  const LONG read_result =
      RegQueryValueExW(scoped_key.get(), L"EDID", nullptr, &type,
                       reinterpret_cast<LPBYTE>(bytes.data()), &read_size);
  if (read_result != ERROR_SUCCESS || type != REG_BINARY || read_size == 0) {
    return std::nullopt;
  }

  bytes.resize(read_size);
  return bytes;
}

}  // namespace

namespace edid {

std::optional<Bytes> GetEdidBytesFromMonitorDevicePath(
    const std::string_view& monitorDevicePath) {
  try {
    if (monitorDevicePath.empty()) {
      return std::nullopt;
    }

    const HDEVINFO raw_dev_info_set =
        SetupDiGetClassDevsW(&GUID_DEVINTERFACE_MONITOR, nullptr, nullptr,
                             DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (raw_dev_info_set == INVALID_HANDLE_VALUE) {
      return std::nullopt;
    }

    ScopedDevInfoSet dev_info_set(raw_dev_info_set);

    for (DWORD interface_index = 0;; ++interface_index) {
      SP_DEVICE_INTERFACE_DATA interface_data = {};
      interface_data.cbSize = sizeof(interface_data);

      if (!SetupDiEnumDeviceInterfaces(dev_info_set.get(), nullptr,
                                       &GUID_DEVINTERFACE_MONITOR,
                                       interface_index, &interface_data)) {
        if (GetLastError() == ERROR_NO_MORE_ITEMS) {
          break;
        }
        return std::nullopt;
      }

      DWORD required_size = 0;
      if (SetupDiGetDeviceInterfaceDetailW(dev_info_set.get(), &interface_data,
                                           nullptr, 0, &required_size,
                                           nullptr) != FALSE) {
        return std::nullopt;
      }

      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
          required_size < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) {
        return std::nullopt;
      }

      std::vector<std::uint8_t> detail_buffer(required_size);
      auto* detail_data = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(
          detail_buffer.data());
      detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

      SP_DEVINFO_DATA dev_info_data = {};
      dev_info_data.cbSize = sizeof(dev_info_data);

      if (!SetupDiGetDeviceInterfaceDetailW(dev_info_set.get(), &interface_data,
                                            detail_data, required_size, nullptr,
                                            &dev_info_data)) {
        return std::nullopt;
      }

      const std::wstring candidate_path(detail_data->DevicePath);
      if (!EqualsIgnoreCase(candidate_path, Utf8ToWide(monitorDevicePath))) {
        continue;
      }

      return ReadEdidBytes(dev_info_set.get(), dev_info_data);
    }

    return std::nullopt;
  } catch (...) {
    return std::nullopt;
  }
}

}  // namespace edid
