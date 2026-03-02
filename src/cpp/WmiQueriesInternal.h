#pragma once

#include "CommonTypes.h"

namespace wmi::internal {

NormalizedJoinKey NormalizeJoinKeyFromDevicePath(
    const DevicePath& monitor_device_path);

bool InstanceNameMatches(const WmiInstanceName& instance_name,
                         const NormalizedJoinKey& normalized_join_key);

bool IsPreferredInstanceName(const WmiInstanceName& instance_name,
                             const NormalizedJoinKey& normalized_join_key);

}  // namespace wmi::internal
