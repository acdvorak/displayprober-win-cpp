#include "DisplayProberInternal.h"

#include <wincrypt.h>

#include <cstdint>
#include <vector>

#include "StringUtils.h"

namespace dp::internal {

std::string TryToExtractShortLivedIdentifier(std::string_view input) {
  constexpr std::string_view kPrefix = R"(\\.\)";
  if (StartsWith(input, kPrefix)) {
    input.remove_prefix(kPrefix.size());
  }

  if (input == "WinDisc" || StartsWith(input, "DISPLAY")) {
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

  constexpr std::string_view kBackslashPrefix = R"(DISPLAY\)";
  if (StartsWith(input, kBackslashPrefix)) {
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
  if (StartsWith(input, kHashPrefix)) {
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

std::string BuildPrimaryPortKey(const ccd::CcdDisplayConfig& display) {
  const std::string gpu_identity =
      HasValue(display.adapter_instance_id)
          ? *display.adapter_instance_id
          : display.adapter_device_path.value_or("");
  if (gpu_identity.empty()) {
    return {};
  }

  auto tpid = "0x" + IntsToHex(display.target_path_id);

  return "acd_ppk:gpu_id=" + gpu_identity + ";tpid=" + tpid;
}

std::string BuildMonitorPathKey(const DevicePath& monitor_device_path) {
  if (monitor_device_path.empty()) {
    return {};
  }

  return "acd_mpk:mdp=" + monitor_device_path;
}

std::string BuildEdidParsedKey(
    const std::optional<json::WinEdidInfo>& edid_info,
    const std::optional<std::string>& monitor_serial_string) {
  if (!edid_info) {
    return {};
  }

  const auto& edid = *edid_info;
  if (!edid.manufacturer_vid || !edid.product_code_id) {
    return {};
  }

  auto vid = ToUpperAscii(*edid.manufacturer_vid);
  auto pid = "0x" + IntsToHex(u16(*edid.product_code_id));

  std::string result = "acd_edid:vid=" + vid + ";pid=" + pid;

  bool has_serial = false;
  if (edid.serial_number_id && *edid.serial_number_id != 0) {
    auto sn = "0x" + IntsToHex(u32(*edid.serial_number_id));
    result += ";sn=" + sn;
    has_serial = true;
  }

  if (monitor_serial_string && !monitor_serial_string->empty()) {
    result += ";ss=" + *monitor_serial_string;
    has_serial = true;
  }

  if (!has_serial) {
    return {};
  }

  return result;
}

std::string BuildLocationPortKey(const json::WinDisplay& json_obj) {
  if (!json_obj.target_path_id) {
    return {};
  }

  std::string location;
  for (const auto& catalog : json_obj.setup_api_devices) {
    for (const auto& adapter : catalog.adapters) {
      if (adapter.location_paths && !adapter.location_paths->empty()) {
        location = adapter.location_paths->front();
        break;
      }
    }
    if (!location.empty()) break;
  }

  if (location.empty()) {
    return {};
  }

  auto tpid = "0x" + IntsToHex(*json_obj.target_path_id);
  return "acd_lpk:loc=" + location + ";tpid=" + tpid;
}

std::string BuildEdidHashKey(const std::vector<std::uint8_t>& edid_bytes) {
  if (edid_bytes.size() < 128) {
    return {};
  }

  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;
  std::string result;

  if (CryptAcquireContextW(&hProv, nullptr, nullptr, PROV_RSA_AES,
                           CRYPT_VERIFYCONTEXT)) {
    if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
      if (CryptHashData(hHash, edid_bytes.data(), 128, 0)) {
        DWORD hash_len = 0;
        DWORD hash_len_size = sizeof(DWORD);
        if (CryptGetHashParam(hHash, HP_HASHSIZE,
                              reinterpret_cast<BYTE*>(&hash_len),
                              &hash_len_size, 0)) {
          std::vector<BYTE> hash(hash_len);
          if (CryptGetHashParam(hHash, HP_HASHVAL, hash.data(), &hash_len, 0)) {
            result =
                "acd_edid:sha256=" + BytesToHexUpper(hash.data(), hash.size());
            auto pos = result.find("=");
            if (pos != std::string::npos) {
              for (size_t i = pos + 1; i < result.size(); ++i) {
                result[i] = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(result[i])));
              }
            }
          }
        }
      }
      CryptDestroyHash(hHash);
    }
    CryptReleaseContext(hProv, 0);
  }

  return result;
}

void ParseEdidStrings(const std::vector<std::uint8_t>& edid_bytes,
                      json::WinDisplay& display) {
  if (edid_bytes.size() < 128) {
    return;
  }

  const size_t offsets[] = {54, 72, 90, 108};
  for (size_t offset : offsets) {
    if (offset + 18 > edid_bytes.size()) continue;

    const auto* desc = &edid_bytes[offset];
    if (desc[0] == 0x00 && desc[1] == 0x00 && desc[2] == 0x00) {
      uint8_t tag = desc[3];
      if (tag == 0xFF || tag == 0xFC || tag == 0xFE) {
        std::string str;
        for (int i = 5; i < 18; ++i) {
          if (desc[i] == 0x0A) break;
          str.push_back(static_cast<char>(desc[i]));
        }
        while (!str.empty() && str.back() == 0x20) {
          str.pop_back();
        }
        if (str.empty()) continue;

        if (tag == 0xFF) {
          if (!display.monitor_serial_string ||
              display.monitor_serial_string->empty()) {
            display.monitor_serial_string = str;
          }
        } else if (tag == 0xFC) {
          if (!display.edid_info) {
            display.edid_info = json::WinEdidInfo{};
          }
          if (!display.edid_info->user_friendly_name ||
              display.edid_info->user_friendly_name->empty()) {
            display.edid_info->user_friendly_name = str;
          }
        } else if (tag == 0xFE) {
          if (!display.edid_data_string || display.edid_data_string->empty()) {
            display.edid_data_string = str;
          }
        }
      }
    }
  }
}

void PopulateStableKeyFields(
    json::WinDisplay& json_obj, const std::optional<std::string>& edid_hash_key,
    const std::optional<std::string>& location_port_key) {
  json::WinStableIds stable_ids;

  auto add_port_independent = [&](const std::optional<std::string>& key,
                                  json::WinStableIdSource source) {
    if (key && !key->empty()) {
      stable_ids.port_independent.push_back({*key, source});
    }
  };

  auto add_port_specific = [&](const std::optional<std::string>& key,
                               json::WinStableIdSource source) {
    if (key && !key->empty()) {
      stable_ids.port_specific.push_back({*key, source});
    }
  };

  add_port_independent(json_obj.edid_parsed_key,
                       json::WinStableIdSource::EDID_PARSED_KEY);
  add_port_independent(edid_hash_key, json::WinStableIdSource::EDID_HASH_KEY);

  add_port_specific(location_port_key,
                    json::WinStableIdSource::LOCATION_PORT_KEY);
  add_port_specific(json_obj.primary_port_key,
                    json::WinStableIdSource::PRIMARY_PORT_KEY);
  add_port_specific(json_obj.monitor_path_key,
                    json::WinStableIdSource::MONITOR_PATH_KEY);

  json_obj.stable_ids = stable_ids;

  // Backward compatibility fields
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

  // Same order as before for flat list compatibility: primary_port,
  // monitor_path, edid
  push_unique(json_obj.primary_port_key);
  push_unique(json_obj.monitor_path_key);
  push_unique(json_obj.edid_parsed_key);

  if (candidates.empty()) {
    return;
  }

  json_obj.stable_id_candidates = candidates;
  json_obj.stable_id = candidates.front();

  auto ppk = json_obj.primary_port_key.value_or("");
  if (!ppk.empty() && ppk == candidates[0]) {
    json_obj.stable_id_source = json::WinStableIdSource::PRIMARY_PORT_KEY;
    return;
  }

  auto mpk = json_obj.monitor_path_key.value_or("");
  if (!mpk.empty() && mpk == candidates[0]) {
    json_obj.stable_id_source = json::WinStableIdSource::MONITOR_PATH_KEY;
    return;
  }

  auto edk = json_obj.edid_parsed_key.value_or("");
  if (!edk.empty() && edk == candidates[0]) {
    json_obj.stable_id_source = json::WinStableIdSource::EDID_PARSED_KEY;
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
