#include "WmiQueries.h"

// This header needs to be imported first.
#include <windows.h>

// Keep other .h headers separate from Windows.h to prevent auto-sorting.
#include <d3dkmthk.h>
#include <mi.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include "EdidBytes.h"
#include "GdiDisplayConfig.h"
#include "StringUtils.h"

namespace {

// Example monitor_device_path:
// `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
NormalizedJoinKey NormalizeJoinKeyFromDevicePath(
    const DevicePath& monitor_device_path);

// Example instance_name:
// `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153_0"`
//
// Example normalized_join_key:
// `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153"`
bool InstanceNameMatches(const WmiInstanceName& instance_name,
                         const NormalizedJoinKey& normalized_join_key);

std::optional<std::string> Uint16ArrayToString(const MI_Instance* inst,
                                               const char* propName);

std::optional<std::uint32_t> Uint16ArrayToU32Decimal(const MI_Instance* inst,
                                                     const char* propName);

std::string MiCharToString(const MI_Char* value) {
  if (value == nullptr) {
    return {};
  }

  std::string out;
#if (MI_CHAR_TYPE == 1)
  while (*value != '\0') {
    out.push_back(*value);
    ++value;
  }
#else
  while (*value != L'\0') {
    out.push_back(static_cast<char>(*value));
    ++value;
  }
#endif
  return out;
}

std::basic_string<MI_Char> ToMiString(const std::string& value) {
  std::basic_string<MI_Char> out;
  out.reserve(value.size());
  for (const char ch : value) {
    out.push_back(static_cast<MI_Char>(ch));
  }
  return out;
}

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

class ScopedMiApplication {
 public:
  ~ScopedMiApplication() { Close(); }

  MI_Result Initialize() {
    return MI_Application_Initialize(0, nullptr, nullptr, &application_);
  }

  MI_Application* get() { return &application_; }

 private:
  void Close() {
    if (application_.ft != nullptr) {
      MI_Application_Close(&application_);
      application_ = {};
    }
  }

  MI_Application application_ = {};
};

class ScopedMiSession {
 public:
  ~ScopedMiSession() { Close(); }

  MI_Result Open(MI_Application* application) {
    return MI_Application_NewSession(application, nullptr, nullptr, nullptr,
                                     nullptr, nullptr, &session_);
  }

  MI_Session* get() { return &session_; }

 private:
  void Close() {
    if (session_.ft != nullptr) {
      MI_Session_Close(&session_, nullptr, nullptr);
      session_ = {};
    }
  }

  MI_Session session_ = {};
};

class ScopedMiOperation {
 public:
  ~ScopedMiOperation() { Close(); }

  MI_Operation* get() { return &operation_; }

 private:
  void Close() {
    if (operation_.ft != nullptr) {
      MI_Operation_Close(&operation_);
      operation_ = {};
    }
  }

  MI_Operation operation_ = {};
};

bool TryGetElement(const MI_Instance* inst, const char* propName,
                   MI_Value* value, MI_Type* type) {
  if (inst == nullptr || propName == nullptr || value == nullptr ||
      type == nullptr) {
    return false;
  }

  const auto miPropName = ToMiString(propName);
  MI_Uint32 flags = 0;
  const MI_Result result = MI_Instance_GetElement(inst, miPropName.c_str(),
                                                  value, type, &flags, nullptr);
  return result == MI_RESULT_OK && (flags & MI_FLAG_NULL) == 0;
}

std::optional<std::uint32_t> GetUint32Property(const MI_Instance* inst,
                                               const char* propName) {
  MI_Value value = {};
  MI_Type type = MI_BOOLEAN;
  if (!TryGetElement(inst, propName, &value, &type)) {
    return std::nullopt;
  }

  switch (type) {
    case MI_UINT8:
      return value.uint8;
    case MI_UINT16:
      return value.uint16;
    case MI_UINT32:
      return value.uint32;
    case MI_UINT64:
      if (value.uint64 <= (std::numeric_limits<std::uint32_t>::max)()) {
        return static_cast<std::uint32_t>(value.uint64);
      }
      return std::nullopt;
    case MI_SINT8:
      if (value.sint8 >= 0) {
        return static_cast<std::uint32_t>(value.sint8);
      }
      return std::nullopt;
    case MI_SINT16:
      if (value.sint16 >= 0) {
        return static_cast<std::uint32_t>(value.sint16);
      }
      return std::nullopt;
    case MI_SINT32:
      if (value.sint32 >= 0) {
        return static_cast<std::uint32_t>(value.sint32);
      }
      return std::nullopt;
    case MI_SINT64:
      if (value.sint64 >= 0 &&
          static_cast<MI_Uint64>(value.sint64) <=
              (std::numeric_limits<std::uint32_t>::max)()) {
        return static_cast<std::uint32_t>(value.sint64);
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<double> GetNumericPropertyAsDouble(const MI_Instance* inst,
                                                 const char* propName) {
  MI_Value value = {};
  MI_Type type = MI_BOOLEAN;
  if (!TryGetElement(inst, propName, &value, &type)) {
    return std::nullopt;
  }

  switch (type) {
    case MI_UINT8:
      return static_cast<double>(value.uint8);
    case MI_UINT16:
      return static_cast<double>(value.uint16);
    case MI_UINT32:
      return static_cast<double>(value.uint32);
    case MI_UINT64:
      return static_cast<double>(value.uint64);
    case MI_SINT8:
      return static_cast<double>(value.sint8);
    case MI_SINT16:
      return static_cast<double>(value.sint16);
    case MI_SINT32:
      return static_cast<double>(value.sint32);
    case MI_SINT64:
      return static_cast<double>(value.sint64);
    case MI_REAL32:
      return static_cast<double>(value.real32);
    case MI_REAL64:
      return value.real64;
    default:
      return std::nullopt;
  }
}

std::optional<std::string> GetStringProperty(const MI_Instance* inst,
                                             const char* propName) {
  MI_Value value = {};
  MI_Type type = MI_BOOLEAN;
  if (!TryGetElement(inst, propName, &value, &type) || type != MI_STRING ||
      value.string == nullptr) {
    return std::nullopt;
  }

  std::string text = MiCharToString(value.string);
  if (text.empty()) {
    return std::nullopt;
  }

  return text;
}

// Example instance_name:
// `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153_0"`
//
// Example normalized_join_key:
// `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153"`
bool IsPreferredInstanceName(const WmiInstanceName& instance_name,
                             const NormalizedJoinKey& normalized_join_key) {
  return AsciiEqualsCaseInsensitive(instance_name, normalized_join_key + "_0");
}

// Example normalized_join_key:
// `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153"`
template <typename ApplyMatchFn>
void QueryClassForBestMatchingInstance(
    MI_Session* session, const char* class_name,
    const NormalizedJoinKey& normalized_join_key,
    const ApplyMatchFn& apply_match) {
  if (session == nullptr || class_name == nullptr ||
      normalized_join_key.empty()) {
    return;
  }

  const std::string query = "SELECT * FROM " + std::string(class_name);
  const auto miQuery = ToMiString(query);

  ScopedMiOperation operation;
  MI_Session_QueryInstances(session, 0, nullptr, MI_T("root\\wmi"), MI_T("WQL"),
                            miQuery.c_str(), nullptr, operation.get());

  const MI_Instance* instance = nullptr;
  MI_Boolean more_results = MI_FALSE;
  MI_Result result = MI_RESULT_OK;

  bool has_any_match = false;
  bool has_preferred_match = false;

  while (true) {
    const MI_Result get_result = MI_Operation_GetInstance(
        operation.get(), &instance, &more_results, &result, nullptr, nullptr);
    if (get_result != MI_RESULT_OK) {
      return;
    }

    if (instance != nullptr && !has_preferred_match) {
      const auto instance_name = GetStringProperty(instance, "InstanceName");
      if (instance_name.has_value() &&
          InstanceNameMatches(*instance_name, normalized_join_key)) {
        const bool is_preferred =
            IsPreferredInstanceName(*instance_name, normalized_join_key);
        if (!has_any_match || is_preferred) {
          apply_match(instance, *instance_name);
          has_any_match = true;
          has_preferred_match = is_preferred;
        }
      }
    }

    if (!more_results) {
      return;
    }
  }
}

// Example monitor_device_path:
// `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
NormalizedJoinKey NormalizeJoinKeyFromDevicePath(
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

// Example instance_name:
// `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153_0"`
//
// Example normalized_join_key:
// `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153"`
bool InstanceNameMatches(const WmiInstanceName& instance_name,
                         const NormalizedJoinKey& normalized_join_key) {
  if (normalized_join_key.empty() ||
      !AsciiStartsWithCaseInsensitive(instance_name, normalized_join_key)) {
    return false;
  }

  if (instance_name.size() == normalized_join_key.size()) {
    return true;
  }

  if (instance_name[normalized_join_key.size()] != '_') {
    return false;
  }

  const std::size_t suffix_start = normalized_join_key.size() + 1;
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

std::optional<std::string> Uint16ArrayToString(const MI_Instance* inst,
                                               const char* propName) {
  MI_Value value = {};
  MI_Type type = MI_BOOLEAN;
  if (!TryGetElement(inst, propName, &value, &type) || type != MI_UINT16A) {
    return std::nullopt;
  }

  const MI_Uint16A& array = value.uint16a;
  if (array.data == nullptr || array.size == 0) {
    return std::nullopt;
  }

  std::string text;
  text.reserve(array.size);
  for (MI_Uint32 i = 0; i < array.size; ++i) {
    const MI_Uint16 code = array.data[i];
    if (code == 0) {
      break;
    }
    text.push_back(static_cast<char>(code));
  }

  if (text.empty()) {
    return std::nullopt;
  }

  return text;
}

std::optional<std::uint32_t> Uint16ArrayToU32Decimal(const MI_Instance* inst,
                                                     const char* propName) {
  const auto text = Uint16ArrayToString(inst, propName);
  if (!text.has_value()) {
    return std::nullopt;
  }

  std::uint64_t parsed = 0;
  for (const char ch : *text) {
    if (!std::isdigit(static_cast<unsigned char>(ch))) {
      return std::nullopt;
    }
    parsed = parsed * 10ULL + static_cast<std::uint64_t>(ch - '0');
    if (parsed > (std::numeric_limits<std::uint32_t>::max)()) {
      return std::nullopt;
    }
  }

  return static_cast<std::uint32_t>(parsed);
}

}  // namespace

namespace wmi {

// Example monitor_device_path:
// `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
std::optional<json::WinEdidInfo> GetWinEdidInfoFromDevicePath(
    const DevicePath& monitor_device_path) {
  const HMODULE mi_module = LoadLibraryW(L"mi.dll");
  if (mi_module == nullptr) {
    return std::nullopt;
  }
  FreeLibrary(mi_module);

  json::WinEdidInfo info{};

  info.monitor_device_path = monitor_device_path;

  // Example normalized_join_key:
  // `"DISPLAY\\SAM7346\\5&21e6c3e1&0&UID5243153"`
  info.normalized_join_key =
      NormalizeJoinKeyFromDevicePath(monitor_device_path);

  if (info.normalized_join_key.empty()) {
    return std::nullopt;
  }

  ScopedMiApplication application;
  if (application.Initialize() != MI_RESULT_OK) {
    return std::nullopt;
  }

  ScopedMiSession session;
  if (session.Open(application.get()) != MI_RESULT_OK) {
    return std::nullopt;
  }

  bool has_populated_data = false;

  QueryClassForBestMatchingInstance(
      session.get(), "WmiMonitorBasicDisplayParams", info.normalized_join_key,
      [&info, &has_populated_data](const MI_Instance* inst,
                                   const WmiInstanceName&) {
        if (const auto horizontal_cm =
                GetNumericPropertyAsDouble(inst, "MaxHorizontalImageSize");
            horizontal_cm.has_value()) {
          info.max_horizontal_image_size_mm = *horizontal_cm * 10.0;
          has_populated_data = true;
        }

        if (const auto vertical_cm =
                GetNumericPropertyAsDouble(inst, "MaxVerticalImageSize");
            vertical_cm.has_value()) {
          info.max_vertical_image_size_mm = *vertical_cm * 10.0;
          has_populated_data = true;
        }
      });

  QueryClassForBestMatchingInstance(
      session.get(), "WmiMonitorConnectionParams", info.normalized_join_key,
      [&info, &has_populated_data](const MI_Instance* inst,
                                   const WmiInstanceName&) {
        if (const auto value = GetUint32Property(inst, "VideoOutputTechnology");
            value.has_value() && *value >= D3DKMDT_VOT_UNINITIALIZED &&
            *value <= D3DKMDT_VOT_INTERNAL) {
          info.video_output_technology_type = static_cast<int64_t>(*value);
          has_populated_data = true;
        }
      });

  QueryClassForBestMatchingInstance(
      session.get(), "WmiMonitorID", info.normalized_join_key,
      [&info, &has_populated_data](const MI_Instance* inst,
                                   const WmiInstanceName& instance_name) {
        info.wmi_instance_name = instance_name;
        has_populated_data = true;

        if (const auto value = Uint16ArrayToString(inst, "ManufacturerName");
            value.has_value()) {
          info.manufacturer_vid = value;
          has_populated_data = true;
        }
        if (const auto value = Uint16ArrayToU32Decimal(inst, "ProductCodeID");
            value.has_value()) {
          info.product_code_id = value;
          has_populated_data = true;
        }
        if (const auto value = Uint16ArrayToU32Decimal(inst, "SerialNumberID");
            value.has_value()) {
          info.serial_number_id = value;
          has_populated_data = true;
        }
        if (const auto value = Uint16ArrayToString(inst, "UserFriendlyName");
            value.has_value()) {
          info.user_friendly_name = value;
          has_populated_data = true;
        }

        if (const auto value = GetUint32Property(inst, "WeekOfManufacture");
            value.has_value()) {
          info.week_of_manufacture = value;
          has_populated_data = true;
        }
        if (const auto value = GetUint32Property(inst, "YearOfManufacture");
            value.has_value()) {
          info.year_of_manufacture = value;
          has_populated_data = true;
        }
      });

  auto bytes = edid::GetEdidBytesFromMonitorDevicePath(monitor_device_path);
  if (bytes.has_value() && !bytes.value().empty()) {
    std::string base64 = Base64Encode(bytes.value());
    info.edid_bytes_base64 = base64;
  }

  if (!has_populated_data) {
    return std::nullopt;
  }

  return info;
}

}  // namespace wmi
