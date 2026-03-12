#include "SetupApiDevice.h"

// This header needs to be imported first.
#include <Windows.h>

// Must be included before devpkey.h
#include <initguid.h>

// Keep other .h headers separate from Windows.h to prevent auto-sorting.
#include <SetupAPI.h>
#include <devpkey.h>
#include <ntddvdeo.h>
#include <objbase.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "CommonTypes.h"
#include "OptionalUtils.h"
#include "ScopedHandles.h"
#include "StringUtils.h"
#include "SysUtils.h"

namespace {

using PFN_SetupDiGetDevicePropertyW =
    BOOL(WINAPI*)(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
                  const DEVPROPKEY* PropertyKey, DEVPROPTYPE* PropertyType,
                  PBYTE PropertyBuffer, DWORD PropertyBufferSize,
                  PDWORD RequiredSize, DWORD Flags);

PFN_SetupDiGetDevicePropertyW GetSetupDiGetDevicePropertyW() {
  static auto* pfn =
      reinterpret_cast<PFN_SetupDiGetDevicePropertyW>(GetProcAddress(
          GetModuleHandleW(L"setupapi.dll"), "SetupDiGetDevicePropertyW"));
  return pfn;
}

std::string FormatGuid(const GUID& guid) {
  WCHAR szGuid[40] = {0};
  StringFromGUID2(guid, szGuid, 40);
  return WideToUtf8(szGuid);
}

std::vector<std::string> ParseMultiSz(std::vector<BYTE>& buffer) {
  std::vector<std::string> result;
  if (buffer.empty()) {
    return result;
  }
  // Ensure double null termination for safety.
  buffer.push_back(0);
  buffer.push_back(0);
  buffer.push_back(0);
  buffer.push_back(0);

  const WCHAR* start = reinterpret_cast<const WCHAR*>(buffer.data());
  const WCHAR* end = start + buffer.size() / sizeof(WCHAR) - 2;
  while (start < end && *start != L'\0') {
    std::wstring str(start);
    if (!str.empty()) {
      result.push_back(WideToUtf8(str));
    }
    start += str.length() + 1;
  }
  return result;
}

struct DeviceInfoHandle {
  std::shared_ptr<void> dev_info_set;
  SP_DEVINFO_DATA dev_info_data;
  DevicePath device_path_mixed_case;
  std::optional<InstanceId> instance_id;
};

std::vector<DeviceInfoHandle> GetDeviceInfoHandlesForClass(
    const GUID* class_guid) {
  std::vector<DeviceInfoHandle> dev_infos;

  try {
    const HDEVINFO raw_dev_info_set = SetupDiGetClassDevsW(
        class_guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (raw_dev_info_set == INVALID_HANDLE_VALUE) {
      return dev_infos;
    }

    std::shared_ptr<void> shared_dev_info_set(raw_dev_info_set,
                                              DevInfoSetDeleter{});

    for (DWORD interface_index = 0;; ++interface_index) {
      SP_DEVICE_INTERFACE_DATA interface_data = {};
      interface_data.cbSize = sizeof(interface_data);

      if (!SetupDiEnumDeviceInterfaces(raw_dev_info_set, nullptr, class_guid,
                                       interface_index, &interface_data)) {
        if (GetLastError() == ERROR_NO_MORE_ITEMS) {
          break;
        }
        return dev_infos;
      }

      DWORD required_size = 0;
      if (SetupDiGetDeviceInterfaceDetailW(raw_dev_info_set, &interface_data,
                                           nullptr, 0, &required_size,
                                           nullptr) != FALSE) {
        continue;
      }

      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
          required_size < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) {
        continue;
      }

      std::vector<std::uint8_t> detail_buffer(required_size);
      auto* detail_data = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(
          detail_buffer.data());
      detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

      SP_DEVINFO_DATA dev_info_data = {};
      dev_info_data.cbSize = sizeof(dev_info_data);

      if (!SetupDiGetDeviceInterfaceDetailW(raw_dev_info_set, &interface_data,
                                            detail_data, required_size, nullptr,
                                            &dev_info_data)) {
        continue;
      }

      // According to Microsoft, there is no official API contract that
      // `SP_DEVICE_INTERFACE_DETAIL_DATA_W.DevicePath` will always be
      // lowercase.
      //
      // In fact, Microsoft explicitly warns that `DevicePath` should be treated
      // as "opaque" and only used for case-insensitive string comparisons,
      // never parsed.
      //
      // However, I have observed that the value *is* lowercase in practice.
      DevicePath device_path_mixed_case = WideToUtf8(detail_data->DevicePath);
      DeviceInfoHandle dev_info{};

      dev_info.dev_info_set = shared_dev_info_set;
      dev_info.dev_info_data = dev_info_data;
      dev_info.device_path_mixed_case = device_path_mixed_case;
      dev_info.instance_id = std::nullopt;

      // https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinstanceidw#return-value
      // The function returns TRUE if it is successful. Otherwise, it returns
      // FALSE and the logged error can be retrieved by making a call to
      // GetLastError.

      DWORD instance_id_length = 0;
      const bool is_len_success = SetupDiGetDeviceInstanceIdW(
          raw_dev_info_set, &dev_info_data, nullptr, 0, &instance_id_length);
      if (!is_len_success) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
            instance_id_length == 0) {
          instance_id_length = 0;
        }
      }

      if (instance_id_length > 0) {
        std::wstring instance_id(instance_id_length, L'\0');

        const bool is_data_success = SetupDiGetDeviceInstanceIdW(
            raw_dev_info_set, &dev_info_data, instance_id.data(),
            instance_id_length, nullptr);

        if (is_data_success) {
          if (!instance_id.empty() && instance_id.back() == L'\0') {
            instance_id.pop_back();
          }
          dev_info.instance_id = WideToUtf8(instance_id);
        }
      }

      dev_infos.push_back(dev_info);
    }

    return dev_infos;
  } catch (...) {
    return dev_infos;
  }
}

template <typename T>
std::optional<T> ExtractProperty(HDEVINFO dev_info_set,
                                 SP_DEVINFO_DATA* dev_info_data,
                                 DWORD spdrp_code,
                                 const DEVPROPKEY* devpropkey) {
  DWORD required_size = 0;
  DWORD property_type = 0;
  DEVPROPTYPE devprop_type = 0;

  bool use_devprop = false;
  auto* pfn_GetDeviceProperty = GetSetupDiGetDevicePropertyW();
  if (sys::is_win_vista_or_newer() && devpropkey != nullptr &&
      pfn_GetDeviceProperty != nullptr) {
    use_devprop = true;
    pfn_GetDeviceProperty(dev_info_set, dev_info_data, devpropkey,
                          &devprop_type, nullptr, 0, &required_size, 0);
  } else {
    SetupDiGetDeviceRegistryPropertyW(dev_info_set, dev_info_data, spdrp_code,
                                      &property_type, nullptr, 0,
                                      &required_size);
  }

  if (required_size == 0) {
    return std::nullopt;
  }

  std::vector<BYTE> buffer(required_size);

  if (use_devprop) {
    if (!pfn_GetDeviceProperty(dev_info_set, dev_info_data, devpropkey,
                               &devprop_type, buffer.data(),
                               static_cast<DWORD>(buffer.size()),
                               &required_size, 0)) {
      return std::nullopt;
    }
  } else {
    if (!SetupDiGetDeviceRegistryPropertyW(
            dev_info_set, dev_info_data, spdrp_code, &property_type,
            buffer.data(), static_cast<DWORD>(buffer.size()), &required_size)) {
      return std::nullopt;
    }
  }

  if constexpr (std::is_same_v<T, std::string>) {
    if (use_devprop) {
      if (devprop_type == DEVPROP_TYPE_STRING) {
        buffer.push_back(0);
        buffer.push_back(0);
        const WCHAR* str = reinterpret_cast<const WCHAR*>(buffer.data());
        return WideToUtf8(str);
      } else if (devprop_type == DEVPROP_TYPE_GUID) {
        if (buffer.size() >= sizeof(GUID)) {
          const GUID* guid = reinterpret_cast<const GUID*>(buffer.data());
          return FormatGuid(*guid);
        }
      }
    } else {
      if (property_type == REG_SZ) {
        buffer.push_back(0);
        buffer.push_back(0);
        const WCHAR* str = reinterpret_cast<const WCHAR*>(buffer.data());
        return WideToUtf8(str);
      } else if (property_type == REG_BINARY) {
        if (buffer.size() >= sizeof(GUID)) {
          const GUID* guid = reinterpret_cast<const GUID*>(buffer.data());
          return FormatGuid(*guid);
        }
      }
    }
  } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
    if (use_devprop && devprop_type == DEVPROP_TYPE_STRING_LIST) {
      return ParseMultiSz(buffer);
    } else if (!use_devprop && property_type == REG_MULTI_SZ) {
      return ParseMultiSz(buffer);
    }
  } else if constexpr (std::is_same_v<T, std::uint32_t>) {
    if (use_devprop && devprop_type == DEVPROP_TYPE_UINT32) {
      if (buffer.size() >= sizeof(std::uint32_t)) {
        std::uint32_t val;
        std::memcpy(&val, buffer.data(), sizeof(val));
        return val;
      }
    } else if (!use_devprop && property_type == REG_DWORD) {
      if (buffer.size() >= sizeof(std::uint32_t)) {
        std::uint32_t val;
        std::memcpy(&val, buffer.data(), sizeof(val));
        return val;
      }
    }
  }

  return std::nullopt;
}

std::optional<Bytes> ReadEdidBytes(HDEVINFO dev_info_set,
                                   SP_DEVINFO_DATA dev_info_data) {
  const HKEY registry_key = SetupDiOpenDevRegKey(
      dev_info_set, &dev_info_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
  if (registry_key == nullptr || registry_key == INVALID_HANDLE_VALUE) {
    return std::nullopt;
  }

  UniqueRegKey unique_key(registry_key);

  DWORD type = 0;
  DWORD size = 0;
  const LONG query_size_result = RegQueryValueExW(
      unique_key.get(), L"EDID", nullptr, &type, nullptr, &size);
  if (query_size_result != ERROR_SUCCESS || type != REG_BINARY || size == 0) {
    return std::nullopt;
  }

  Bytes bytes(size);
  DWORD read_size = size;
  type = 0;
  const LONG read_result =
      RegQueryValueExW(unique_key.get(), L"EDID", nullptr, &type,
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

  std::vector<DeviceInfoHandle> dev_infos =
      GetDeviceInfoHandlesForClass(class_guid);

  for (const DeviceInfoHandle& dev_info : dev_infos) {
    if (EqualsIgnoreCase(dev_info.device_path_mixed_case, target_device_path)) {
      return dev_info.instance_id;
    }
  }

  return std::nullopt;
}

}  // namespace

namespace setupapi {

std::optional<Bytes> GetEdidBytesFromMonitorDevicePath(
    std::string_view monitor_device_path) {
  if (monitor_device_path.empty()) {
    return std::nullopt;
  }

  std::vector<DeviceInfoHandle> dev_infos =
      GetDeviceInfoHandlesForClass(&GUID_DEVINTERFACE_MONITOR);

  for (const DeviceInfoHandle& dev_info : dev_infos) {
    if (EqualsIgnoreCase(dev_info.device_path_mixed_case,
                         monitor_device_path)) {
      return ReadEdidBytes(dev_info.dev_info_set.get(), dev_info.dev_info_data);
    }
  }

  return std::nullopt;
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

  UniqueDevInfoSet dev_info(raw_dev_info_handle);

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

  buffer.push_back(0);
  buffer.push_back(0);
  const WCHAR* driver_property = reinterpret_cast<const WCHAR*>(buffer.data());
  return WideToUtf8(driver_property);
}

// TODO(acdvorak): Create a new function that enumerates all SetupAPI devices
// and returns a map of ShortLivedIdentifier (MONITORINFOEX.szDevice).
json::WinSetupApiDevice GetDeviceProperties(HDEVINFO dev_info_set,
                                            SP_DEVINFO_DATA* dev_info_data) {
  json::WinSetupApiDevice props;
  props.device_desc = ExtractProperty<std::string>(dev_info_set, dev_info_data,
                                                   SPDRP_DEVICEDESC,
                                                   &DEVPKEY_Device_DeviceDesc);
  props.hardware_id = ExtractProperty<std::vector<std::string>>(
      dev_info_set, dev_info_data, SPDRP_HARDWAREID,
      &DEVPKEY_Device_HardwareIds);
  props.compatible_ids = ExtractProperty<std::vector<std::string>>(
      dev_info_set, dev_info_data, SPDRP_COMPATIBLEIDS,
      &DEVPKEY_Device_CompatibleIds);
  props.service = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_SERVICE, &DEVPKEY_Device_Service);
  props.class_name = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_CLASS, &DEVPKEY_Device_Class);
  props.class_guid = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_CLASSGUID, &DEVPKEY_Device_ClassGuid);
  props.driver = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_DRIVER, &DEVPKEY_Device_Driver);
  props.config_flags = ExtractProperty<std::uint32_t>(
      dev_info_set, dev_info_data, SPDRP_CONFIGFLAGS,
      &DEVPKEY_Device_ConfigFlags);
  props.mfg = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_MFG, &DEVPKEY_Device_Manufacturer);
  props.friendly_name = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_FRIENDLYNAME,
      &DEVPKEY_Device_FriendlyName);
  props.location_information = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_LOCATION_INFORMATION,
      &DEVPKEY_Device_LocationInfo);
  props.physical_device_object_name = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
      &DEVPKEY_Device_PDOName);
  props.capabilities = ExtractProperty<std::uint32_t>(
      dev_info_set, dev_info_data, SPDRP_CAPABILITIES,
      &DEVPKEY_Device_Capabilities);
  props.ui_number = ExtractProperty<std::uint32_t>(
      dev_info_set, dev_info_data, SPDRP_UI_NUMBER, &DEVPKEY_Device_UINumber);
  props.bus_type_guid = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_BUSTYPEGUID,
      &DEVPKEY_Device_BusTypeGuid);
  props.legacy_bus_type = ExtractProperty<std::uint32_t>(
      dev_info_set, dev_info_data, SPDRP_LEGACYBUSTYPE,
      &DEVPKEY_Device_LegacyBusType);
  props.bus_number = ExtractProperty<std::uint32_t>(
      dev_info_set, dev_info_data, SPDRP_BUSNUMBER, &DEVPKEY_Device_BusNumber);
  props.enumerator_name = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_ENUMERATOR_NAME,
      &DEVPKEY_Device_EnumeratorName);
  props.dev_type = ExtractProperty<std::uint32_t>(
      dev_info_set, dev_info_data, SPDRP_DEVTYPE, &DEVPKEY_Device_DevType);
  props.characteristics = ExtractProperty<std::uint32_t>(
      dev_info_set, dev_info_data, SPDRP_CHARACTERISTICS,
      &DEVPKEY_Device_Characteristics);
  props.address = ExtractProperty<std::uint32_t>(
      dev_info_set, dev_info_data, SPDRP_ADDRESS, &DEVPKEY_Device_Address);
  props.ui_number_desc_format = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_UI_NUMBER_DESC_FORMAT,
      &DEVPKEY_Device_UINumberDescFormat);
  props.location_paths = ExtractProperty<std::vector<std::string>>(
      dev_info_set, dev_info_data, SPDRP_LOCATION_PATHS,
      &DEVPKEY_Device_LocationPaths);
  props.base_container_id = ExtractProperty<std::string>(
      dev_info_set, dev_info_data, SPDRP_BASE_CONTAINERID,
      &DEVPKEY_Device_BaseContainerId);

  return props;
}

json::WinSetupApiDeviceCatalog GetAllSetupApiDevices() {
  auto raw_adapter_handles =
      GetDeviceInfoHandlesForClass(&GUID_DEVINTERFACE_DISPLAY_ADAPTER);

  auto raw_monitor_handles =
      GetDeviceInfoHandlesForClass(&GUID_DEVINTERFACE_MONITOR);

  json::WinSetupApiDeviceCatalog datas;

  for (DeviceInfoHandle& handle : raw_adapter_handles) {
    json::WinSetupApiDevice json =
        GetDeviceProperties(handle.dev_info_set.get(), &handle.dev_info_data);
    json.device_path_mixed_case = handle.device_path_mixed_case;
    json.instance_id = handle.instance_id;
    datas.adapters.push_back(json);
  }

  for (DeviceInfoHandle& handle : raw_monitor_handles) {
    json::WinSetupApiDevice json =
        GetDeviceProperties(handle.dev_info_set.get(), &handle.dev_info_data);
    json.device_path_mixed_case = handle.device_path_mixed_case;
    json.instance_id = handle.instance_id;
    datas.monitors.push_back(json);
  }

  return datas;
}

}  // namespace setupapi
