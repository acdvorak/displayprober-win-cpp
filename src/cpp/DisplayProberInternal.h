#pragma once

// This header needs to be imported first.
#include <Windows.h>

#include <optional>
#include <string>
#include <string_view>

#include "CommonTypes.h"
#include "GdiDisplayConfig.h"
#include "gencode/acd-json.hpp"

namespace dp::internal {

std::string TryToExtractShortLivedIdentifier(std::string_view input);

std::string TryToExtractEdid7DigitIdentifier(std::string_view input);

std::string BuildPrimaryPortKey(const gdi::GdiDisplayConfig& config);

std::string BuildMonitorPathKey(const DevicePath& monitor_device_path);

std::string BuildEdidKey(const std::optional<json::WinEdidInfo>& edid_info);

void PopulateStableKeyFields(json::WinDisplay& json_obj);

bool RectHasZeroWidthOrHeight(std::optional<RECT> rect);

}  // namespace dp::internal
