#pragma once

// This header needs to be imported first.
#include <Windows.h>

#include <optional>
#include <string>
#include <string_view>

#include "CcdDisplayConfig.h"
#include "CommonTypes.h"
#include "gencode/acd-json.hpp"

namespace dp::internal {

std::string TryToExtractShortLivedIdentifier(std::string_view input);

std::string TryToExtractEdid7DigitIdentifier(std::string_view input);

void ParseEdidStrings(const std::vector<std::uint8_t>& edid_bytes,
                      json::WinDisplay& display);

std::string BuildPrimaryPortKey(const ccd::CcdDisplayConfig& display);

std::string BuildMonitorPathKey(const DevicePath& monitor_device_path);

std::string BuildLocationPortKey(const json::WinDisplay& json_obj);

std::string BuildEdidParsedKey(
    const std::optional<json::WinEdidInfo>& edid_info,
    const std::optional<std::string>& monitor_serial_string);

std::string BuildEdidHashKey(const std::vector<std::uint8_t>& edid_bytes);

void PopulateStableKeyFields(
    json::WinDisplay& json_obj, const std::optional<std::string>& edid_hash_key,
    const std::optional<std::string>& location_port_key);

bool RectHasZeroWidthOrHeight(std::optional<RECT> rect);

}  // namespace dp::internal
