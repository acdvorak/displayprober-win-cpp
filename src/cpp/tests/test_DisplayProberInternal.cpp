#include <doctest/doctest.h>

#include <cstdint>

#include "DisplayProberInternal.h"

TEST_CASE("TryToExtractShortLivedIdentifier accepts known forms") {
  CHECK(dp::internal::TryToExtractShortLivedIdentifier(R"(\\.\DISPLAY1)") ==
        "DISPLAY1");
  CHECK(dp::internal::TryToExtractShortLivedIdentifier("DISPLAY") == "DISPLAY");
  CHECK(dp::internal::TryToExtractShortLivedIdentifier("WinDisc") == "WinDisc");
  CHECK(
      dp::internal::TryToExtractShortLivedIdentifier("not-a-display").empty());
}

TEST_CASE("TryToExtractEdid7DigitIdentifier extracts valid IDs") {
  CHECK(dp::internal::TryToExtractEdid7DigitIdentifier(
            R"(DISPLAY\SAM73A5\5&21e6c3e1&0&UID5243153_0)") == "SAM73A5");
  CHECK(
      dp::internal::TryToExtractEdid7DigitIdentifier(
          R"(\\?\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7})") ==
      "DELF023");
  CHECK(dp::internal::TryToExtractEdid7DigitIdentifier(R"(DISPLAY\BAD$123\5&x)")
            .empty());
  CHECK(dp::internal::TryToExtractEdid7DigitIdentifier(R"(DISPLAY\SHORT\5&x)")
            .empty());
}

TEST_CASE("BuildPrimaryPortKey builds deterministic key") {
  gdi::GdiDisplayConfig config{};
  config.adapter_instance_id =
      "PCI\\VEN_10DE&DEV_2684&SUBSYS_16E110DE&REV_A1\\4&2A5F5B12&0&0008";
  config.target_path_id = 42;

  CHECK(dp::internal::BuildPrimaryPortKey(config) ==
        "acd_ppk:gpu_id=PCI\\VEN_10DE&DEV_2684&SUBSYS_16E110DE&REV_A1\\4&"
        "2A5F5B12&0&0008;tp_id=0x0000002A");

  config.adapter_instance_id.clear();
  config.adapter_device_path = "\\\\?\\pci#ven_10de#device-path";
  CHECK(dp::internal::BuildPrimaryPortKey(config) ==
        "acd_ppk:gpu_id=\\\\?\\pci#ven_10de#device-path;tp_id=0x0000002A");
}

TEST_CASE("BuildEdidKey requires all parts and normalizes VID") {
  json::WinEdidInfo info{};
  info.manufacturer_vid = "sam";
  info.product_code_id = static_cast<std::uint16_t>(0x23);
  info.serial_number_id = static_cast<std::uint32_t>(1);

  CHECK(dp::internal::BuildEdidKey(info) ==
        "acd_edid:vid=SAM;pid=0x0023;sn=0x00000001");

  info.serial_number_id = std::nullopt;
  CHECK(dp::internal::BuildEdidKey(info).empty());
}

TEST_CASE("PopulateStableKeyFields applies priority and dedupes") {
  json::WinDisplay display{};
  display.primary_port_key = "ppk";
  display.monitor_path_key = "mpk";
  display.edid_key = "edk";

  dp::internal::PopulateStableKeyFields(display);

  REQUIRE(display.stable_id.has_value());
  CHECK(*display.stable_id == "ppk");
  REQUIRE(display.stable_id_source.has_value());
  CHECK(*display.stable_id_source == json::StableIdSource::PRIMARY_PORT_KEY);
  REQUIRE(display.stable_id_candidates.has_value());
  REQUIRE(display.stable_id_candidates->size() == 3);
  CHECK((*display.stable_id_candidates)[0] == "ppk");
  CHECK((*display.stable_id_candidates)[1] == "mpk");
  CHECK((*display.stable_id_candidates)[2] == "edk");

  json::WinDisplay deduped{};
  deduped.primary_port_key = "same";
  deduped.monitor_path_key = "same";
  deduped.edid_key = "other";

  dp::internal::PopulateStableKeyFields(deduped);

  REQUIRE(deduped.stable_id_candidates.has_value());
  REQUIRE(deduped.stable_id_candidates->size() == 2);
  CHECK((*deduped.stable_id_candidates)[0] == "same");
  CHECK((*deduped.stable_id_candidates)[1] == "other");
}
