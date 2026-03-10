#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "CommonTypes.h"

namespace pnp {

std::optional<Bytes> GetEdidBytesFromMonitorDevicePath(
    std::string_view monitor_device_path);

std::optional<std::string> TryGetAdapterInstanceIdFromAdapterPath(
    const std::optional<std::string>& adapter_device_path);

std::optional<std::string> TryGetMonitorInstanceIdFromMonitorPath(
    const std::string& monitor_device_path);

std::optional<std::string> TryGetMonitorDriverKeyFromDeviceInstanceId(
    const std::string& device_instance_id);

}  // namespace pnp
