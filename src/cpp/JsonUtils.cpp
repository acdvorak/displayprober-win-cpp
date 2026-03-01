#include "JsonUtils.h"

// This header needs to be imported first.
#include <Windows.h>

// Keep other .h headers separate from Windows.h to prevent auto-sorting.
#include <dxgi.h>

#include <cstdint>

#include "GdiPolyfills.h"
#include "SysUtils.h"
#include "gencode/acd-json.hpp"

namespace json_utils {

void PopulateRectangleIfZero(json::WinScreenRectangle& bounds, RECT& rect) {
  if (bounds.x == 0) {
    bounds.x = static_cast<int32_t>(rect.left);
  }
  if (bounds.y == 0) {
    bounds.y = static_cast<int32_t>(rect.top);
  }
  if (bounds.width == 0) {
    bounds.width = static_cast<uint32_t>(rect.right - rect.left);
  }
  if (bounds.height == 0) {
    bounds.height = static_cast<uint32_t>(rect.bottom - rect.top);
  }
  if (bounds.left == 0) {
    bounds.left = static_cast<int32_t>(rect.left);
  }
  if (bounds.top == 0) {
    bounds.top = static_cast<int32_t>(rect.top);
  }
  if (bounds.right == 0) {
    bounds.right = static_cast<int32_t>(rect.right);
  }
  if (bounds.bottom == 0) {
    bounds.bottom = static_cast<int32_t>(rect.bottom);
  }
}

std::optional<json::WinColorEncoding> ColorEncodingToJson(
    std::optional<DISPLAYCONFIG_COLOR_ENCODING> color_encoding) {
  if (!color_encoding.has_value()) {
    return std::nullopt;
  }
  switch (*color_encoding) {
    case DISPLAYCONFIG_COLOR_ENCODING_RGB:
      return json::WinColorEncoding::RGB;
    case DISPLAYCONFIG_COLOR_ENCODING_YCBCR444:
      return json::WinColorEncoding::YCBCR444;
    case DISPLAYCONFIG_COLOR_ENCODING_YCBCR422:
      return json::WinColorEncoding::YCBCR422;
    case DISPLAYCONFIG_COLOR_ENCODING_YCBCR420:
      return json::WinColorEncoding::YCBCR420;
    default:
      return json::WinColorEncoding::UNSPECIFIED;
  }
}

std::optional<json::WinDisplayConnectorType> OutputTechnologyToJson(
    std::optional<DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY> output_technology) {
  if (!output_technology) {
    return std::nullopt;
  }
  switch (*output_technology) {
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HD15:
      return json::WinDisplayConnectorType::VGA;
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SVIDEO:
      return json::WinDisplayConnectorType::SVIDEO;
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_COMPOSITE_VIDEO:
      return json::WinDisplayConnectorType::COMPOSITE_VIDEO;
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_COMPONENT_VIDEO:
      return json::WinDisplayConnectorType::COMPONENT_VIDEO;
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DVI:
      return json::WinDisplayConnectorType::DVI;
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI:
      return json::WinDisplayConnectorType::HDMI;
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_LVDS:
      return json::WinDisplayConnectorType::LVDS;
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_D_JPN
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_D_JPN:
      return json::WinDisplayConnectorType::D_JPN;
#endif
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SDI
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SDI:
      return json::WinDisplayConnectorType::SDI;
#endif
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EXTERNAL
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EXTERNAL:
      return json::WinDisplayConnectorType::DISPLAYPORT_EXTERNAL;
#endif
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED:
      return json::WinDisplayConnectorType::DISPLAYPORT_EMBEDDED;
#endif
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EXTERNAL
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EXTERNAL:
      return json::WinDisplayConnectorType::UDI_EXTERNAL;
#endif
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED:
      return json::WinDisplayConnectorType::UDI_EMBEDDED;
#endif
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SDTVDONGLE
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SDTVDONGLE:
      return json::WinDisplayConnectorType::SDTVDONGLE;
#endif
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_MIRACAST
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_MIRACAST:
      return json::WinDisplayConnectorType::MIRACAST;
#endif
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_WIRED
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_WIRED:
      return json::WinDisplayConnectorType::INDIRECT_WIRED;
#endif
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_VIRTUAL
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_VIRTUAL:
      return json::WinDisplayConnectorType::INDIRECT_VIRTUAL;
#endif
#ifdef DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_USB_TUNNEL
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_USB_TUNNEL:
      return json::WinDisplayConnectorType::DISPLAYPORT_USB_TUNNEL;
#endif
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL:
      return json::WinDisplayConnectorType::INTERNAL;
    case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_OTHER:
    default:
      return sys::IsRdpSession() ? json::WinDisplayConnectorType::RDP
                                 : json::WinDisplayConnectorType::OTHER;
  }
}

std::optional<json::WinScanLineOrder> ScanLineOrderingToJson(
    std::optional<DISPLAYCONFIG_SCANLINE_ORDERING> scan_line_ordering) {
  if (!scan_line_ordering.has_value()) {
    return std::nullopt;
  }
  switch (*scan_line_ordering) {
    case DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE:
      return json::WinScanLineOrder::PROGRESSIVE;
    case DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED:
      return json::WinScanLineOrder::INTERLACED_UPPER_FIELD_FIRST;
    case DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED_LOWERFIELDFIRST:
      return json::WinScanLineOrder::INTERLACED_LOWER_FIELD_FIRST;
    case DISPLAYCONFIG_SCANLINE_ORDERING_UNSPECIFIED:
      return json::WinScanLineOrder::UNSPECIFIED;
    default:
      return std::nullopt;
  }
}

std::optional<json::WinActiveColorMode> ActiveColorModeToJson(
    std::optional<DISPLAYCONFIG_ADVANCED_COLOR_MODE> color_mode) {
  if (!color_mode.has_value()) {
    return std::nullopt;
  }
  switch (*color_mode) {
    case DISPLAYCONFIG_ADVANCED_COLOR_MODE_SDR:
      return json::WinActiveColorMode::SDR;
    case DISPLAYCONFIG_ADVANCED_COLOR_MODE_WCG:
      return json::WinActiveColorMode::WCG;
    case DISPLAYCONFIG_ADVANCED_COLOR_MODE_HDR:
      return json::WinActiveColorMode::HDR;
    default:
      return json::WinActiveColorMode::UNSPECIFIED;
  }
}

std::optional<json::WinDxgiColorSpace> DxgiColorSpaceToJson(
    std::optional<DXGI_COLOR_SPACE_TYPE> color_space) {
  if (!color_space.has_value()) {
    return std::nullopt;
  }
  switch (*color_space) {
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
      return json::WinDxgiColorSpace::RGB_FULL_G22_NONE_P709;
    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
      return json::WinDxgiColorSpace::RGB_FULL_G10_NONE_P709;
    case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709:
      return json::WinDxgiColorSpace::RGB_STUDIO_G22_NONE_P709;
    case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020:
      return json::WinDxgiColorSpace::RGB_STUDIO_G22_NONE_P2020;
    case DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601:
      return json::WinDxgiColorSpace::YCBCR_FULL_G22_NONE_P709_X601;
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601:
      return json::WinDxgiColorSpace::YCBCR_STUDIO_G22_LEFT_P601;
    case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601:
      return json::WinDxgiColorSpace::YCBCR_FULL_G22_LEFT_P601;
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709:
      return json::WinDxgiColorSpace::YCBCR_STUDIO_G22_LEFT_P709;
    case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709:
      return json::WinDxgiColorSpace::YCBCR_FULL_G22_LEFT_P709;
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020:
      return json::WinDxgiColorSpace::YCBCR_STUDIO_G22_LEFT_P2020;
    case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020:
      return json::WinDxgiColorSpace::YCBCR_FULL_G22_LEFT_P2020;
    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
      return json::WinDxgiColorSpace::RGB_FULL_G2084_NONE_P2020;
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020:
      return json::WinDxgiColorSpace::YCBCR_STUDIO_G2084_LEFT_P2020;
    case DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020:
      return json::WinDxgiColorSpace::RGB_STUDIO_G2084_NONE_P2020;
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020:
      return json::WinDxgiColorSpace::YCBCR_STUDIO_G22_TOPLEFT_P2020;
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020:
      return json::WinDxgiColorSpace::YCBCR_STUDIO_G2084_TOPLEFT_P2020;
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020:
      return json::WinDxgiColorSpace::RGB_FULL_G22_NONE_P2020;
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020:
      return json::WinDxgiColorSpace::YCBCR_STUDIO_GHLG_TOPLEFT_P2020;
    case DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020:
      return json::WinDxgiColorSpace::YCBCR_FULL_GHLG_TOPLEFT_P2020;
    case DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709:
      return json::WinDxgiColorSpace::RGB_STUDIO_G24_NONE_P709;
    case DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020:
      return json::WinDxgiColorSpace::RGB_STUDIO_G24_NONE_P2020;
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709:
      return json::WinDxgiColorSpace::YCBCR_STUDIO_G24_LEFT_P709;
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020:
      return json::WinDxgiColorSpace::YCBCR_STUDIO_G24_LEFT_P2020;
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020:
      return json::WinDxgiColorSpace::YCBCR_STUDIO_G24_TOPLEFT_P2020;
    case DXGI_COLOR_SPACE_RESERVED:
      return json::WinDxgiColorSpace::RESERVED;
#ifdef DXGI_COLOR_SPACE_CUSTOM
    case DXGI_COLOR_SPACE_CUSTOM:
      return json::WinDxgiColorSpace::CUSTOM;
#endif
    default:
      return std::nullopt;
  }
}

std::optional<json::WinDisplayRotationDegrees> DxgiRotationToJson(
    std::optional<DXGI_MODE_ROTATION> rotation_type) {
  if (!rotation_type.has_value()) {
    return std::nullopt;
  }
  switch (*rotation_type) {
    case DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_IDENTITY:
      return json::WinDisplayRotationDegrees::VALUE_0;
    case DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_ROTATE90:
      return json::WinDisplayRotationDegrees::VALUE_90;
    case DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_ROTATE180:
      return json::WinDisplayRotationDegrees::VALUE_180;
    case DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_ROTATE270:
      return json::WinDisplayRotationDegrees::VALUE_270;
    case DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_UNSPECIFIED:
    default:
      return std::nullopt;
  }
}

}  // namespace json_utils
