#include <doctest/doctest.h>

#include "WmiQueriesInternal.h"

TEST_CASE("NormalizeJoinKeyFromDevicePath handles valid and invalid values") {
  CHECK(
      wmi::internal::NormalizeJoinKeyFromDevicePath(
          R"(\\?\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7})") ==
      R"(DISPLAY\SAM7346\5&21e6c3e1&0&UID5243153)");

  CHECK(wmi::internal::NormalizeJoinKeyFromDevicePath(
            R"(DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153)")
            .empty());

  CHECK(wmi::internal::NormalizeJoinKeyFromDevicePath(
            R"(\\?\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153)")
            .empty());
}

TEST_CASE("InstanceNameMatches follows suffix and prefix rules") {
  const std::string key = R"(DISPLAY\SAM7346\5&21e6c3e1&0&UID5243153)";

  CHECK(wmi::internal::InstanceNameMatches(key, key));
  CHECK(wmi::internal::InstanceNameMatches(key + "_0", key));
  CHECK(wmi::internal::InstanceNameMatches(key + "_12", key));
  CHECK(wmi::internal::InstanceNameMatches(
      R"(display\sam7346\5&21e6c3e1&0&uid5243153_7)", key));

  CHECK_FALSE(wmi::internal::InstanceNameMatches(key + "_", key));
  CHECK_FALSE(wmi::internal::InstanceNameMatches(key + "_A", key));
  CHECK_FALSE(wmi::internal::InstanceNameMatches(key + "X", key));
}

TEST_CASE("IsPreferredInstanceName matches case-insensitive _0 instance") {
  const std::string key = R"(DISPLAY\SAM7346\5&21e6c3e1&0&UID5243153)";

  CHECK(wmi::internal::IsPreferredInstanceName(key + "_0", key));
  CHECK(wmi::internal::IsPreferredInstanceName(
      R"(display\sam7346\5&21e6c3e1&0&uid5243153_0)", key));
  CHECK_FALSE(wmi::internal::IsPreferredInstanceName(key + "_1", key));
}
