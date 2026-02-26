#pragma once

#include <optional>
#include <string>

#include "CommonTypes.h"
#include "gencode/acd-json.hpp"

namespace wmi {

// Example device_path:
// `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
std::optional<json::WinEdidInfo> GetWinEdidInfoFromDevicePath(
    const DevicePath& device_path);

}  // namespace wmi
