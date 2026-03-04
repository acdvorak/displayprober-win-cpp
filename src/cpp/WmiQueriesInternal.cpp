#include "WmiQueriesInternal.h"

#include <cctype>

namespace wmi::internal {

namespace {

bool AsciiStartsWithCaseInsensitive(const std::string& value,
                                    const std::string& prefix) {
  if (prefix.size() > value.size()) {
    return false;
  }

  for (std::size_t i = 0; i < prefix.size(); ++i) {
    const auto lhs = static_cast<unsigned char>(value[i]);
    const auto rhs = static_cast<unsigned char>(prefix[i]);
    if (std::tolower(lhs) != std::tolower(rhs)) {
      return false;
    }
  }

  return true;
}

bool AsciiEqualsCaseInsensitive(const std::string& lhs,
                                const std::string& rhs) {
  return lhs.size() == rhs.size() && AsciiStartsWithCaseInsensitive(lhs, rhs);
}

}  // namespace

// Example monitor_device_path:
// `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
//
// Example return value:
// `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153"`
WmiJoinKey NormalizeJoinKeyFromDevicePath(
    const DevicePath& monitor_device_path) {
  constexpr const char* kPrefix = "\\\\?\\DISPLAY#";
  const std::size_t prefix_pos = monitor_device_path.find(kPrefix);
  if (prefix_pos == std::string::npos) {
    return {};
  }

  const std::size_t start = prefix_pos + std::string(kPrefix).size();
  const std::size_t end = monitor_device_path.find("#{", start);
  if (end == std::string::npos || end <= start) {
    return {};
  }

  std::string tail = monitor_device_path.substr(start, end - start);
  for (char& ch : tail) {
    if (ch == '#') {
      ch = '\\';
    }
  }

  return "DISPLAY\\" + tail;
}

bool InstanceNameMatches(const WmiInstanceName& instance_name,
                         const WmiJoinKey& wmi_join_key) {
  if (wmi_join_key.empty() ||
      !AsciiStartsWithCaseInsensitive(instance_name, wmi_join_key)) {
    return false;
  }

  if (instance_name.size() == wmi_join_key.size()) {
    return true;
  }

  if (instance_name[wmi_join_key.size()] != '_') {
    return false;
  }

  const std::size_t suffix_start = wmi_join_key.size() + 1;
  if (suffix_start >= instance_name.size()) {
    return false;
  }

  for (std::size_t i = suffix_start; i < instance_name.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(instance_name[i]))) {
      return false;
    }
  }

  return true;
}

bool IsPreferredInstanceName(const WmiInstanceName& instance_name,
                             const WmiJoinKey& wmi_join_key) {
  return AsciiEqualsCaseInsensitive(instance_name, wmi_join_key + "_0");
}

}  // namespace wmi::internal
