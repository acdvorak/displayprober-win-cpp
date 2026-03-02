#include <doctest/doctest.h>

#include "JsonUtils.h"

TEST_CASE("PopulateRectangleIfZero only fills zero-valued fields") {
  json::WinScreenRectangle bounds{};
  bounds.left = 100;

  RECT rect = {10, 20, 210, 120};
  json_utils::PopulateRectangleIfZero(bounds, rect);

  CHECK(bounds.x == 10);
  CHECK(bounds.y == 20);
  CHECK(bounds.width == 200);
  CHECK(bounds.height == 100);
  CHECK(bounds.left == 100);
  CHECK(bounds.top == 20);
  CHECK(bounds.right == 210);
  CHECK(bounds.bottom == 120);
}

TEST_CASE("Core enum mapping helpers return expected values") {
  CHECK_FALSE(json_utils::ColorEncodingToJson(std::nullopt).has_value());
  CHECK(json_utils::ColorEncodingToJson(DISPLAYCONFIG_COLOR_ENCODING_RGB) ==
        json::WinColorEncoding::RGB);
  CHECK(json_utils::ColorEncodingToJson(
            static_cast<DISPLAYCONFIG_COLOR_ENCODING>(9999)) ==
        json::WinColorEncoding::UNSPECIFIED);

  CHECK_FALSE(json_utils::ScanLineOrderingToJson(std::nullopt).has_value());
  CHECK(json_utils::ScanLineOrderingToJson(
            DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE) ==
        json::WinScanLineOrder::PROGRESSIVE);
  CHECK_FALSE(json_utils::ScanLineOrderingToJson(
                  static_cast<DISPLAYCONFIG_SCANLINE_ORDERING>(9999))
                  .has_value());

  CHECK_FALSE(json_utils::ActiveColorModeToJson(std::nullopt).has_value());
  CHECK(json_utils::ActiveColorModeToJson(
            DISPLAYCONFIG_ADVANCED_COLOR_MODE_HDR) ==
        json::WinActiveColorMode::HDR);
  CHECK(json_utils::ActiveColorModeToJson(
            static_cast<DISPLAYCONFIG_ADVANCED_COLOR_MODE>(9999)) ==
        json::WinActiveColorMode::UNSPECIFIED);
}

TEST_CASE("DXGI mapping helpers handle known and unknown values") {
  CHECK_FALSE(json_utils::DxgiColorSpaceToJson(std::nullopt).has_value());
  CHECK(json_utils::DxgiColorSpaceToJson(
            DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709) ==
        json::WinDxgiColorSpace::RGB_FULL_G22_NONE_P709);
  CHECK_FALSE(
      json_utils::DxgiColorSpaceToJson(static_cast<DXGI_COLOR_SPACE_TYPE>(9999))
          .has_value());

  CHECK_FALSE(json_utils::DxgiRotationToJson(std::nullopt).has_value());
  CHECK(json_utils::DxgiRotationToJson(DXGI_MODE_ROTATION_IDENTITY) ==
        json::WinDisplayRotationDegrees::VALUE_0);
  CHECK_FALSE(json_utils::DxgiRotationToJson(DXGI_MODE_ROTATION_UNSPECIFIED)
                  .has_value());
}
