#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "CommonTypes.h"

namespace edid {

std::optional<Bytes> GetEdidBytesFromMonitorDevicePath(
    const std::string_view& monitorDevicePath);

}  // namespace edid
