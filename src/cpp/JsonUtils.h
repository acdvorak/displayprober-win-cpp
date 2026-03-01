#pragma once

// This header needs to be imported first.
#include <Windows.h>

// Keep other .h headers separate from Windows.h to prevent auto-sorting.
#include <dxgi.h>

#include <cstdint>

#include "GdiPolyfills.h"
#include "gencode/acd-json.hpp"

namespace json_utils {

void PopulateRectangleIfZero(json::WinScreenRectangle& bounds, RECT& rect);

std::optional<json::WinColorEncoding> ColorEncodingToJson(
    std::optional<DISPLAYCONFIG_COLOR_ENCODING> color_encoding);

std::optional<json::WinDisplayConnectorType> OutputTechnologyToJson(
    std::optional<DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY> output_technology);

std::optional<json::WinScanLineOrder> ScanLineOrderingToJson(
    std::optional<DISPLAYCONFIG_SCANLINE_ORDERING> scan_line_ordering);

std::optional<json::WinActiveColorMode> ActiveColorModeToJson(
    std::optional<DISPLAYCONFIG_ADVANCED_COLOR_MODE> color_mode);

std::optional<json::WinDxgiColorSpace> DxgiColorSpaceToJson(
    std::optional<DXGI_COLOR_SPACE_TYPE> color_space);

std::optional<json::WinDisplayRotationDegrees> DxgiRotationToJson(
    std::optional<DXGI_MODE_ROTATION> rotation_type);

}  // namespace json_utils
