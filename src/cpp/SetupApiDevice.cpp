#include "SetupApiDevice.h"

// This header needs to be imported first.
#include <Windows.h>

// Keep other .h headers separate from Windows.h to prevent auto-sorting.
#include <SetupAPI.h>
#include <initguid.h>
#include <ntddvdeo.h>

#include <cstdint>
#include <vector>

#include "ScopedHandles.h"
#include "StringUtils.h"

namespace {

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

std::optional<std::string> TryGetInstanceIdFromDevicePath(
    const std::string& target_device_path, const GUID* class_guid) {
  if (target_device_path.empty()) {
    return std::nullopt;
  }

  const HDEVINFO raw_dev_info_set = SetupDiGetClassDevsW(
      class_guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (raw_dev_info_set == INVALID_HANDLE_VALUE) {
    return std::nullopt;
  }

  ScopedDevInfoSet dev_info_set(raw_dev_info_set);
  const std::wstring wanted_path = Utf8ToWide(target_device_path);

  for (DWORD interface_index = 0;; ++interface_index) {
    SP_DEVICE_INTERFACE_DATA interface_data = {};
    interface_data.cbSize = sizeof(interface_data);

    if (!SetupDiEnumDeviceInterfaces(dev_info_set.get(), nullptr, class_guid,
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
    if (!EqualsIgnoreCase(candidate_path, wanted_path)) {
      continue;
    }

    DWORD instance_id_length = 0;
    if (!SetupDiGetDeviceInstanceIdW(dev_info_set.get(), &dev_info_data,
                                     nullptr, 0, &instance_id_length)) {
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
          instance_id_length == 0) {
        return std::nullopt;
      }
    }

    std::wstring instance_id(instance_id_length, L'\0');
    if (!SetupDiGetDeviceInstanceIdW(dev_info_set.get(), &dev_info_data,
                                     instance_id.data(), instance_id_length,
                                     nullptr)) {
      return std::nullopt;
    }

    if (!instance_id.empty() && instance_id.back() == L'\0') {
      instance_id.pop_back();
    }

    const std::string instance_id_utf8 = WideToUtf8(instance_id);
    if (!instance_id_utf8.empty()) {
      return instance_id_utf8;
    }

    return std::nullopt;
  }

  return std::nullopt;
}

}  // namespace

namespace setupapi {

std::optional<Bytes> GetEdidBytesFromMonitorDevicePath(
    std::string_view monitor_device_path) {
  try {
    if (monitor_device_path.empty()) {
      return std::nullopt;
    }

    const HDEVINFO raw_dev_info_set =
        SetupDiGetClassDevsW(&GUID_DEVINTERFACE_MONITOR, nullptr, nullptr,
                             DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (raw_dev_info_set == INVALID_HANDLE_VALUE) {
      return std::nullopt;
    }

    ScopedDevInfoSet dev_info_set(raw_dev_info_set);
    const std::wstring wanted_path = Utf8ToWide(monitor_device_path);

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
      if (!EqualsIgnoreCase(candidate_path, wanted_path)) {
        continue;
      }

      return ReadEdidBytes(dev_info_set.get(), dev_info_data);
    }

    return std::nullopt;
  } catch (...) {
    return std::nullopt;
  }
}

std::optional<std::string> TryGetAdapterInstanceIdFromAdapterPath(
    const std::optional<std::string>& adapter_device_path) {
  if (!HasValue(adapter_device_path)) {
    return std::nullopt;
  }
  return TryGetInstanceIdFromDevicePath(*adapter_device_path,
                                        &GUID_DEVINTERFACE_DISPLAY_ADAPTER);
}

std::optional<std::string> TryGetMonitorInstanceIdFromMonitorPath(
    const std::string& monitor_device_path) {
  return TryGetInstanceIdFromDevicePath(monitor_device_path,
                                        &GUID_DEVINTERFACE_MONITOR);
}

std::optional<std::string> TryGetMonitorDriverKeyFromDeviceInstanceId(
    const std::string& device_instance_id) {
  const HDEVINFO raw_dev_info_handle = SetupDiCreateDeviceInfoList(NULL, NULL);
  if (raw_dev_info_handle == INVALID_HANDLE_VALUE) {
    return std::nullopt;
  }

  ScopedDevInfoSet dev_info(raw_dev_info_handle);

  SP_DEVINFO_DATA dev_info_data = {};
  dev_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

  if (!SetupDiOpenDeviceInfoW(dev_info.get(),
                              Utf8ToWide(device_instance_id).c_str(), NULL, 0,
                              &dev_info_data)) {
    return std::nullopt;
  }

  DWORD property_type = 0;
  DWORD required_size = 0;

  // First call to get the required buffer size for SPDRP_DRIVER.
  if (!SetupDiGetDeviceRegistryPropertyW(dev_info.get(), &dev_info_data,
                                         SPDRP_DRIVER, &property_type, nullptr,
                                         0, &required_size)) {
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || required_size == 0) {
      return std::nullopt;
    }
  }

  std::vector<BYTE> buffer(required_size);
  property_type = 0;
  if (!SetupDiGetDeviceRegistryPropertyW(
          dev_info.get(), &dev_info_data, SPDRP_DRIVER, &property_type,
          buffer.data(), static_cast<DWORD>(buffer.size()), &required_size)) {
    return std::nullopt;
  }

  if (property_type != REG_SZ) {
    return std::nullopt;
  }

  const WCHAR* driver_property = reinterpret_cast<const WCHAR*>(buffer.data());
  return WideToUtf8(driver_property);
}

}  // namespace setupapi
