#pragma once

// This header needs to be imported first.
#include <Windows.h>

// Keep other .h headers separate from Windows.h to prevent auto-sorting.
#include <SetupAPI.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "CommonTypes.h"
#include "gencode/acd-json.hpp"

namespace setupapi {

json::WinSetupApiDeviceCatalog GetAllSetupApiDevices();

std::optional<Bytes> GetEdidBytesFromMonitorDevicePath(
    std::string_view monitor_device_path);

std::optional<std::string> TryGetAdapterInstanceIdFromAdapterPath(
    const std::optional<std::string>& adapter_device_path);

std::optional<std::string> TryGetMonitorInstanceIdFromMonitorPath(
    const std::string& monitor_device_path);

std::optional<std::string> TryGetMonitorDriverKeyFromDeviceInstanceId(
    const std::string& device_instance_id);

}  // namespace setupapi
