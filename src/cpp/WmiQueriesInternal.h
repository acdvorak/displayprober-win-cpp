#pragma once

#include "CommonTypes.h"

namespace wmi::internal {

// Example monitor_device_path:
// `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
//
// Example return value:
// `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153"`
WmiJoinKey NormalizeJoinKeyFromDevicePath(
    const DevicePath& monitor_device_path);

bool InstanceNameMatches(const WmiInstanceName& instance_name,
                         const WmiJoinKey& wmi_join_key);

bool IsPreferredInstanceName(const WmiInstanceName& instance_name,
                             const WmiJoinKey& wmi_join_key);

}  // namespace wmi::internal
