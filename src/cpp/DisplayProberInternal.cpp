#include "DisplayProberInternal.h"

#include <cstdint>
#include <format>
#include <vector>

#include "StringUtils.h"

namespace dp::internal {

std::string TryToExtractShortLivedIdentifier(std::string_view input) {
  constexpr std::string_view kPrefix = R"(\\.\)";
  if (input.starts_with(kPrefix)) {
    input.remove_prefix(kPrefix.size());
  }

  if (input == "WinDisc" || input.starts_with("DISPLAY")) {
    return std::string(input);
  }

  return "";
}

std::string TryToExtractEdid7DigitIdentifier(std::string_view input) {
  auto is_valid_edid_id = [](const std::string_view value) {
    if (value.size() != 7) {
      return false;
    }

    for (const char ch : value) {
      const bool is_digit = (ch >= '0' && ch <= '9');
      const bool is_upper = (ch >= 'A' && ch <= 'Z');
      const bool is_lower = (ch >= 'a' && ch <= 'z');
      if (!is_digit && !is_upper && !is_lower) {
        return false;
      }
    }

    return true;
  };

  constexpr std::string_view kBackslashPrefix = "DISPLAY\\";
  if (input.starts_with(kBackslashPrefix)) {
    const std::size_t begin = kBackslashPrefix.size();
    const std::size_t end = input.find('\\', begin);
    if (end != std::string_view::npos) {
      const std::string_view candidate = input.substr(begin, end - begin);
      if (is_valid_edid_id(candidate)) {
        return std::string(candidate);
      }
    }
  }

  constexpr std::string_view kHashPrefix = R"(\\?\DISPLAY#)";
  if (input.starts_with(kHashPrefix)) {
    const std::size_t begin = kHashPrefix.size();
    const std::size_t end = input.find('#', begin);
    if (end != std::string_view::npos) {
      const std::string_view candidate = input.substr(begin, end - begin);
      if (is_valid_edid_id(candidate)) {
        return std::string(candidate);
      }
    }
  }

  return "";
}

std::string BuildPrimaryPortKey(const gdi::GdiDisplayConfig& config) {
  const std::string gpu_identity =
      !config.adapter_instance_id.empty()
          ? config.adapter_instance_id
          : config.adapter_device_path.value_or("");
  if (gpu_identity.empty()) {
    return {};
  }

  auto tp_id = "0x" + IntsToHex(config.target_path_id);

  return std::format("acd_ppk:gpu_id={};tp_id={}", gpu_identity, tp_id);
}

std::string BuildMonitorPathKey(const DevicePath& monitor_device_path) {
  if (monitor_device_path.empty()) {
    return {};
  }

  return std::format("acd_mpk:mdp={}", monitor_device_path);
}

std::string BuildEdidKey(const std::optional<json::WinEdidInfo>& edid_info) {
  if (!edid_info) {
    return {};
  }

  const auto& info = *edid_info;
  if (!info.manufacturer_vid || !info.product_code_id ||
      !info.serial_number_id) {
    return {};
  }

  auto vid = ToUpperAscii(*info.manufacturer_vid);
  auto pid = "0x" + IntsToHex(static_cast<uint16_t>(*info.product_code_id));
  auto sn = "0x" + IntsToHex(static_cast<uint32_t>(*info.serial_number_id));

  return std::format("acd_edid:vid={};pid={};sn={}", vid, pid, sn);
}

void PopulateStableKeyFields(json::WinDisplay& json_obj) {
  std::vector<std::string> candidates;

  auto push_unique = [&candidates](const std::optional<std::string>& key) {
    if (!key || key->empty()) {
      return;
    }

    for (const auto& existing : candidates) {
      if (existing == *key) {
        return;
      }
    }

    candidates.push_back(*key);
  };

  push_unique(json_obj.primary_port_key);
  push_unique(json_obj.monitor_path_key);
  push_unique(json_obj.edid_key);

  if (candidates.empty()) {
    return;
  }

  json_obj.stable_id_candidates = candidates;
  json_obj.stable_id = candidates.front();

  if (json_obj.primary_port_key &&
      *json_obj.primary_port_key == candidates[0]) {
    json_obj.stable_id_source = json::StableIdSource::PRIMARY_PORT_KEY;
    return;
  }

  if (json_obj.monitor_path_key &&
      *json_obj.monitor_path_key == candidates[0]) {
    json_obj.stable_id_source = json::StableIdSource::MONITOR_PATH_KEY;
    return;
  }

  if (json_obj.edid_key && *json_obj.edid_key == candidates[0]) {
    json_obj.stable_id_source = json::StableIdSource::EDID_KEY;
  }
}

bool RectHasZeroWidthOrHeight(std::optional<RECT> rect) {
  if (!rect) {
    return false;
  }
  auto& r = *rect;
  if (r.top == 0 && r.bottom == 0) {
    return true;
  }
  if (r.left == 0 && r.right == 0) {
    return true;
  }
  return false;
}

}  // namespace dp::internal
