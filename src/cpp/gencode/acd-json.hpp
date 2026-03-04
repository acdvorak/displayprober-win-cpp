//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     WinDisplayProberJson data = nlohmann::json::parse(jsonString);

#pragma once

#include <optional>
#include <nlohmann/json.hpp>

#include <unordered_map>

#ifndef NLOHMANN_OPT_HELPER
#define NLOHMANN_OPT_HELPER
namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::shared_ptr<T>> {
        static void to_json(json & j, const std::shared_ptr<T> & opt) {
            if (!opt) j = nullptr; else j = *opt;
        }

        static std::shared_ptr<T> from_json(const json & j) {
            if (j.is_null()) return std::make_shared<T>(); else return std::make_shared<T>(j.get<T>());
        }
    };
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json & j, const std::optional<T> & opt) {
            if (!opt) j = nullptr; else j = *opt;
        }

        static std::optional<T> from_json(const json & j) {
            if (j.is_null()) return std::make_optional<T>(); else return std::make_optional<T>(j.get<T>());
        }
    };
}
#endif

namespace json {
    using nlohmann::json;

    #ifndef NLOHMANN_UNTYPED_json_HELPER
    #define NLOHMANN_UNTYPED_json_HELPER
    inline json get_untyped(const json & j, const char * property) {
        if (j.find(property) != j.end()) {
            return j.at(property).get<json>();
        }
        return json();
    }

    inline json get_untyped(const json & j, std::string property) {
        return get_untyped(j, property.data());
    }
    #endif

    #ifndef NLOHMANN_OPTIONAL_json_HELPER
    #define NLOHMANN_OPTIONAL_json_HELPER
    template <typename T>
    inline std::shared_ptr<T> get_heap_optional(const json & j, const char * property) {
        auto it = j.find(property);
        if (it != j.end() && !it->is_null()) {
            return j.at(property).get<std::shared_ptr<T>>();
        }
        return std::shared_ptr<T>();
    }

    template <typename T>
    inline std::shared_ptr<T> get_heap_optional(const json & j, std::string property) {
        return get_heap_optional<T>(j, property.data());
    }
    template <typename T>
    inline std::optional<T> get_stack_optional(const json & j, const char * property) {
        auto it = j.find(property);
        if (it != j.end() && !it->is_null()) {
            return j.at(property).get<std::optional<T>>();
        }
        return std::optional<T>();
    }

    template <typename T>
    inline std::optional<T> get_stack_optional(const json & j, std::string property) {
        return get_stack_optional<T>(j, property.data());
    }
    #endif

    enum class WinDisplayRotationDegrees : uint16_t {
        VALUE_0 = 0,
        VALUE_90 = 90,
        VALUE_180 = 180,
        VALUE_270 = 270
    };

    enum class WinBitsPerColorChannel : uint8_t {
        VALUE_0 = 0,
        VALUE_6 = 6,
        VALUE_8 = 8,
        VALUE_10 = 10,
        VALUE_12 = 12,
        VALUE_14 = 14,
        VALUE_16 = 16
    };

    enum class WmiVideoOutputTechnology : int64_t {
        UNINITIALIZED = -2,
        OTHER = -1,
        VGA = 0,
        SVIDEO = 1,
        COMPOSITE_VIDEO = 2,
        COMPONENT_VIDEO = 3,
        DVI = 4,
        HDMI = 5,
        LVDS = 6,
        D_JPN = 8,
        SDI = 9,
        DISPLAYPORT_EXTERNAL = 10,
        DISPLAYPORT_EMBEDDED = 11,
        UDI_EXTERNAL = 12,
        UDI_EMBEDDED = 13,
        SDTVDONGLE = 14,
        MIRACAST = 15,
        INDIRECT_WIRED = 16,
        INTERNAL = 2147483648
    };

    enum class WinActiveColorMode : int { HDR, SDR, UNSPECIFIED, WCG };

    struct WinAdvancedColorInfo {
        std::optional<WinActiveColorMode> active_color_mode;
        bool is_advanced_color_active;
        bool is_advanced_color_enabled;
        bool is_advanced_color_force_disabled;
        bool is_advanced_color_limited_by_policy;
        bool is_advanced_color_supported;
        bool is_high_dynamic_range_supported;
        bool is_high_dynamic_range_user_enabled;
        bool is_wide_color_enforced;
        bool is_wide_color_supported;
        bool is_wide_color_user_enabled;
    };

    /**
     * The full size and position of the display, *including* the taskbar and any other areas
     * that are not usable by maximized (non-fullscreen) applications.
     *
     * Rectangle payload used by Bounds and WorkingArea.
     *
     * Available working area on the screen, *excluding* taskbars and other docked windows.
     */
    struct WinScreenRectangle {
        int32_t bottom;
        uint32_t height;
        int32_t left;
        int32_t right;
        int32_t top;
        uint32_t width;
        int32_t x;
        int32_t y;
    };

    struct WinEdidInfo {
        /**
         * Raw EDID bytes, Base64-encoded.
         */
        std::optional<std::string> edid_bytes_base64;
        /**
         * 3-letter Vendor ID (aka PnP ID).
         *
         * Examples:
         *
         * - `"SAM"` (Samsung)
         * - `"DEL"` (Dell)
         */
        std::optional<std::string> manufacturer_vid;
        std::optional<double> max_horizontal_image_size_mm;
        std::optional<double> max_vertical_image_size_mm;
        /**
         * Corresponds to: `DISPLAYCONFIG_TARGET_DEVICE_NAME.monitorDevicePath`.
         *
         * Examples:
         *
         * -
         * `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
         * -
         * `"\\\\?\\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
         */
        std::string monitor_device_path;
        std::optional<uint16_t> product_code_id;
        std::optional<uint32_t> serial_number_id;
        /**
         * Examples:
         *
         * - `"DELL ST2320L"`
         * - `"QCQ95S"`  // Samsung S95C TV
         * - `"SAMSUNG"` // Some devices don't give us an actual model number
         */
        std::optional<std::string> user_friendly_name;
        /**
         * Raw numeric value that gets mapped to  {@link  WinDisplayConnectorType } .
         */
        std::optional<WmiVideoOutputTechnology> video_output_technology_type;
        std::optional<uint8_t> week_of_manufacture;
        /**
         * Corresponds to the `InstanceName` field of these WMI object classes:
         *
         * - `WmiMonitorBasicDisplayParams`
         * - `WmiMonitorConnectionParams`
         * - `WmiMonitorDescriptorMethods`
         * - `WmiMonitorID`
         * - `WmiMonitorListedSupportedSourceModes`
         *
         * Examples:
         *
         * - `"DISPLAY\\SAM73A5\\5&21e6c3e1&0&UID5243153_0"`
         * - `"DISPLAY\\DELF023\\5&21e6c3e1&0&UID5243152_0"`
         */
        std::optional<std::string> wmi_instance_name;
        /**
         * Normalized join key, with trailing `_0` removed from  {@link  wmi_instance_name } .
         *
         * E.g.:
         *
         * - `"DISPLAY\\SAM73A5\\5&21e6c3e1&0&UID5243153"`
         * - `"DISPLAY\\DELF023\\5&21e6c3e1&0&UID5243152"`
         */
        std::string wmi_join_key;
        std::optional<uint16_t> year_of_manufacture;
    };

    /**
     * Values with "embedded" in their names indicate that the graphics adapter's video output
     * device connects internally to the display device.
     *
     * In those cases, the `DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL` value is redundant. The
     * caller should ignore `DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL` and just process the
     * embedded values, `DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED` and
     * `DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED`.
     *
     * An embedded display port is also known as an integrated display port or UDI.
     */
    enum class WinDisplayConnectorType : int { COMPONENT_VIDEO, COMPOSITE_VIDEO, DISPLAYPORT_EMBEDDED, DISPLAYPORT_EXTERNAL, DISPLAYPORT_USB_TUNNEL, DVI, D_JPN, FAKE, HDMI, INDIRECT_VIRTUAL, INDIRECT_WIRED, INTERNAL, LVDS, MIRACAST, OTHER, RDP, SDI, SDTVDONGLE, SVIDEO, UDI_EMBEDDED, UDI_EXTERNAL, VGA };

    enum class WinScanLineOrder : int { INTERLACED_LOWER_FIELD_FIRST, INTERLACED_UPPER_FIELD_FIRST, PROGRESSIVE, UNSPECIFIED };

    enum class StableIdSource : int { EDID_KEY, MONITOR_PATH_KEY, PRIMARY_PORT_KEY };

    enum class WinColorEncoding : int { RGB, UNSPECIFIED, YCBCR420, YCBCR422, YCBCR444 };

    enum class WinDxgiColorSpace : int { CUSTOM, RESERVED, RGB_FULL_G10_NONE_P709, RGB_FULL_G2084_NONE_P2020, RGB_FULL_G22_NONE_P2020, RGB_FULL_G22_NONE_P709, RGB_STUDIO_G2084_NONE_P2020, RGB_STUDIO_G22_NONE_P2020, RGB_STUDIO_G22_NONE_P709, RGB_STUDIO_G24_NONE_P2020, RGB_STUDIO_G24_NONE_P709, YCBCR_FULL_G22_LEFT_P2020, YCBCR_FULL_G22_LEFT_P601, YCBCR_FULL_G22_LEFT_P709, YCBCR_FULL_G22_NONE_P709_X601, YCBCR_FULL_GHLG_TOPLEFT_P2020, YCBCR_STUDIO_G2084_LEFT_P2020, YCBCR_STUDIO_G2084_TOPLEFT_P2020, YCBCR_STUDIO_G22_LEFT_P2020, YCBCR_STUDIO_G22_LEFT_P601, YCBCR_STUDIO_G22_LEFT_P709, YCBCR_STUDIO_G22_TOPLEFT_P2020, YCBCR_STUDIO_G24_LEFT_P2020, YCBCR_STUDIO_G24_LEFT_P709, YCBCR_STUDIO_G24_TOPLEFT_P2020, YCBCR_STUDIO_GHLG_TOPLEFT_P2020 };

    struct WinStandardColorInfo {
        std::optional<WinBitsPerColorChannel> bits_per_channel;
        std::optional<WinColorEncoding> color_encoding;
        std::optional<WinDxgiColorSpace> dxgi_color_space;
        bool is_hdr_enabled;
        bool is_hdr_supported;
        /**
         * Full-screen sustained luminance.
         *
         * In `nits` - i.e., luminance in candelas per square meter (`cd/m^2`).
         */
        std::optional<double> max_full_frame_luminance_nits;
        /**
         * In `nits` - i.e., luminance in candelas per square meter (`cd/m^2`).
         */
        std::optional<double> max_luminance_nits;
        /**
         * In `nits` - i.e., luminance in candelas per square meter (`cd/m^2`).
         */
        std::optional<double> min_luminance_nits;
    };

    struct WinDisplay {
        /**
         * Persistent across reboots in the common case (same GPU/driver instance).
         *
         * TODO(acdvorak): Describe GPU vs Adapter vs Driver.
         *
         * Corresponds to: `DISPLAYCONFIG_ADAPTER_NAME.adapterDevicePath`
         */
        std::optional<std::string> adapter_device_path;
        /**
         * Adapter Plug and Play instance ID (typically the GPU's unique ID).
         *
         * For a single GPU with two ports (e.g., one DisplayPort and one DVI), where both ports are
         * connected to an active monitor/TV, both displays will have the same
         * `adapter_instance_id`.
         *
         * TODO(acdvorak): Describe GPU vs Adapter vs Driver.
         *
         * More persistent than `adapter_device_path` across reboots and driver churn.
         *
         * Examples:
         *
         * - `"PCI\\VEN_10DE&DEV_0DF8&SUBSYS_083510DE&REV_A1\\4&2B1C6285&0&0010"`
         */
        std::optional<std::string> adapter_instance_id;
        std::optional<WinAdvancedColorInfo> advanced_color_info;
        /**
         * The full size and position of the display, *including* the taskbar and any other areas
         * that are not usable by maximized (non-fullscreen) applications.
         */
        WinScreenRectangle bounds;
        /**
         * The most common standard values are: `100 | 125 | 150 | 175 | 200`.
         *
         * The user can technically set any arbitrary value via registry hacks.
         */
        std::optional<uint32_t> dpi_scaling_percent;
        std::optional<WinEdidInfo> edid_info;
        /**
         * ✅ TERTIARY STABLE ID (when available)
         *
         * Deterministic monitor identity key based on EDID.
         *
         * Only emitted when manufacturer, product code, and serial are available.
         */
        std::optional<std::string> edid_key;
        /**
         * Human-friendly name of the display.
         *
         * Value comes from one of the following sources, in descending order of quality (i.e., the
         * "best" available value is returned):
         *
         * 1. EDID "monitor descriptor" name (e.g., `"DELL ST2320L"`) 2. `"Remote Desktop"` or
         * `"Remote Desktop #N"` if RDP 3. `"Virtual Machine"` or `"Virtual Machine #N"` if a VM 4.
         * 7-digit Windows EDID identifier (e.g., `"SAM73A5"` or `"DELF023"`) 5. Short-lived Windows
         * display number (e.g., `"DISPLAY1"`)
         *
         * Examples:
         *
         * - `"DELL ST2320L"`
         * - `"QCQ95S"`      // Samsung S95C TV
         * - `"SAMSUNG"`     // Some devices don't give us an actual model number
         * - `"SAM73A5"`     // Samsung S95C TV
         * - `"DELF023"`     // Dell ST2320L monitor
         * - `"DISPLAY1"`    // Primary monitor
         * - `"DISPLAY2"`    // Secondary monitor
         * - `"DISPLAY129"`  // RDP monitor
         * - `"DISPLAY"`     // Single monitor
         * - `"WinDisc"`     // Non-interactive remote SSH console session
         */
        std::optional<std::string> friendly_name;
        /**
         * This value MIGHT be `false` under the following conditions:
         *
         * - Unused connectors on the GPU:   - Many drivers expose one IDXGIOutput per physical
         * connector     (HDMI/DP/DVI), even if nothing is plugged in.   - Those "ports" can
         * enumerate, but they are not part of the desktop, so     AttachedToDesktop is false.
         *
         * - A monitor is connected but disabled in Display Settings:   - Example: you have 2
         * monitors connected, but Windows is set to     "Show only on 1" (or you've "Disconnect
         * this display" for the other).     That other output can still exist, but it is not
         * attached, so false.
         *
         * TODO(acdvorak): Clarify the difference between  {@link  has_interactive_desktop  }  and
         * {@link  is_attached_to_desktop } .
         */
        std::optional<bool> is_attached_to_desktop;
        bool is_primary;
        /**
         * Typically stable across reboots and uniquely identifies the monitor instance on that
         * connection path.
         *
         * It is also useful for correlating to EDID retrieval.
         *
         * Corresponds to: `DISPLAYCONFIG_TARGET_DEVICE_NAME.monitorDevicePath`.
         *
         * Examples:
         *
         * -
         * `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
         * -
         * `"\\\\?\\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
         */
        std::optional<std::string> monitor_device_path;
        /**
         * ✅ SECONDARY STABLE ID (when available)
         *
         * Deterministic key derived from  {@link  monitor_device_path } .
         */
        std::optional<std::string> monitor_path_key;
        /**
         * Physical connector type, if applicable (HDMI, DVI, DisplayPort, etc.).
         */
        std::optional<WinDisplayConnectorType> physical_connector_type;
        /**
         * ✅ PRIMARY STABLE ID (when available)
         *
         * Value:
         *
         * ``` (adapter_instance_id ?? adapter_device_path) + target_path_id ```
         */
        std::optional<std::string> primary_port_key;
        std::optional<uint32_t> refresh_rate_denominator;
        /**
         * {@link  refresh_rate_numerator }  /  {@link  refresh_rate_denominator } .
         *
         * Examples:
         *
         * - `60`
         * - `120`
         * - `144`
         */
        std::optional<double> refresh_rate_hz;
        std::optional<uint32_t> refresh_rate_numerator;
        std::optional<WinDisplayRotationDegrees> rotation_deg;
        /**
         * Progressive or interlaced.
         */
        std::optional<WinScanLineOrder> scan_line_ordering;
        /**
         * Windows "monitor device name" from `MONITORINFOEX.szDevice`.
         *
         * ⚠️ NOT stable across device disconnects/reconnects.
         *
         * Examples:
         *
         * - `"\\\\.\\DISPLAY1"`   (multi-monitor)
         * - `"\\\\.\\DISPLAY2"`   (multi-monitor)
         * - `"\\\\.\\DISPLAY129"` (Remote Desktop)
         * - `"DISPLAY"`           (single-monitor)
         * - `"WinDisc"`           (SSH console)
         */
        std::string short_lived_identifier;
        /**
         * Effective stable ID after applying candidate ordering.
         */
        std::optional<std::string> stable_id;
        /**
         * Candidate stable keys ordered from strongest to weakest.
         *
         * 1. `primary_port_key` 2. `monitor_path_key` 3. `edid_key`
         *
         * TODO(acdvorak): Refactor
         */
        std::optional<std::vector<std::string>> stable_id_candidates;
        /**
         * Indicates which candidate produced  {@link  stable_id } .
         *
         * TODO(acdvorak): Refactor
         */
        std::optional<StableIdSource> stable_id_source;
        WinStandardColorInfo standard_color_info;
        /**
         * Corresponds to `DISPLAYCONFIG_PATH_INFO.targetInfo.id`.
         */
        std::optional<uint32_t> target_path_id;
        /**
         * Available working area on the screen, *excluding* taskbars and other docked windows.
         */
        WinScreenRectangle working_area;
    };

    struct WinDisplayProberJson {
        std::vector<WinDisplay> displays;
        /**
         * This is a *session-level* value, not specific to an individual display.
         *
         * It will be `false` in all non-interactive sessions, such as:
         *
         * - SSH
         * - Remote console
         * - Headless server
         *
         * TODO(acdvorak): Clarify the difference between  {@link  has_interactive_desktop }  and
         * {@link  is_attached_to_desktop  } .
         */
        bool has_interactive_desktop;
        /**
         * Indicates whether the current session is Microsoft Remote Desktop (RDP).
         *
         * This is a *session-level* value, not specific to an individual display.
         */
        bool is_remote_desktop;
        /**
         * Best-effort signal that this display is *probably* running in a VM guest.
         *
         * This is a *session-level* value, not specific to an individual display.
         */
        bool is_virtual_machine;
    };
}

namespace json {
    void from_json(const json & j, WinAdvancedColorInfo & x);
    void to_json(json & j, const WinAdvancedColorInfo & x);

    void from_json(const json & j, WinScreenRectangle & x);
    void to_json(json & j, const WinScreenRectangle & x);

    void from_json(const json & j, WinEdidInfo & x);
    void to_json(json & j, const WinEdidInfo & x);

    void from_json(const json & j, WinStandardColorInfo & x);
    void to_json(json & j, const WinStandardColorInfo & x);

    void from_json(const json & j, WinDisplay & x);
    void to_json(json & j, const WinDisplay & x);

    void from_json(const json & j, WinDisplayProberJson & x);
    void to_json(json & j, const WinDisplayProberJson & x);

    void from_json(const json & j, WinActiveColorMode & x);
    void to_json(json & j, const WinActiveColorMode & x);

    void from_json(const json & j, WinDisplayConnectorType & x);
    void to_json(json & j, const WinDisplayConnectorType & x);

    void from_json(const json & j, WinScanLineOrder & x);
    void to_json(json & j, const WinScanLineOrder & x);

    void from_json(const json & j, StableIdSource & x);
    void to_json(json & j, const StableIdSource & x);

    void from_json(const json & j, WinColorEncoding & x);
    void to_json(json & j, const WinColorEncoding & x);

    void from_json(const json & j, WinDxgiColorSpace & x);
    void to_json(json & j, const WinDxgiColorSpace & x);

    void from_json(const json & j, WinDisplayRotationDegrees & x);
    void to_json(json & j, const WinDisplayRotationDegrees & x);

    void from_json(const json & j, WinBitsPerColorChannel & x);
    void to_json(json & j, const WinBitsPerColorChannel & x);

    void from_json(const json & j, WmiVideoOutputTechnology & x);
    void to_json(json & j, const WmiVideoOutputTechnology & x);

    inline void from_json(const json & j, WinAdvancedColorInfo& x) {
        x.active_color_mode = get_stack_optional<WinActiveColorMode>(j, "active_color_mode");
        x.is_advanced_color_active = j.at("is_advanced_color_active").get<bool>();
        x.is_advanced_color_enabled = j.at("is_advanced_color_enabled").get<bool>();
        x.is_advanced_color_force_disabled = j.at("is_advanced_color_force_disabled").get<bool>();
        x.is_advanced_color_limited_by_policy = j.at("is_advanced_color_limited_by_policy").get<bool>();
        x.is_advanced_color_supported = j.at("is_advanced_color_supported").get<bool>();
        x.is_high_dynamic_range_supported = j.at("is_high_dynamic_range_supported").get<bool>();
        x.is_high_dynamic_range_user_enabled = j.at("is_high_dynamic_range_user_enabled").get<bool>();
        x.is_wide_color_enforced = j.at("is_wide_color_enforced").get<bool>();
        x.is_wide_color_supported = j.at("is_wide_color_supported").get<bool>();
        x.is_wide_color_user_enabled = j.at("is_wide_color_user_enabled").get<bool>();
    }

    inline void to_json(json & j, const WinAdvancedColorInfo & x) {
        j = json::object();
        if (x.active_color_mode) {
            j["active_color_mode"] = x.active_color_mode;
        }
        j["is_advanced_color_active"] = x.is_advanced_color_active;
        j["is_advanced_color_enabled"] = x.is_advanced_color_enabled;
        j["is_advanced_color_force_disabled"] = x.is_advanced_color_force_disabled;
        j["is_advanced_color_limited_by_policy"] = x.is_advanced_color_limited_by_policy;
        j["is_advanced_color_supported"] = x.is_advanced_color_supported;
        j["is_high_dynamic_range_supported"] = x.is_high_dynamic_range_supported;
        j["is_high_dynamic_range_user_enabled"] = x.is_high_dynamic_range_user_enabled;
        j["is_wide_color_enforced"] = x.is_wide_color_enforced;
        j["is_wide_color_supported"] = x.is_wide_color_supported;
        j["is_wide_color_user_enabled"] = x.is_wide_color_user_enabled;
    }

    inline void from_json(const json & j, WinScreenRectangle& x) {
        x.bottom = j.at("bottom").get<int32_t>();
        x.height = j.at("height").get<uint32_t>();
        x.left = j.at("left").get<int32_t>();
        x.right = j.at("right").get<int32_t>();
        x.top = j.at("top").get<int32_t>();
        x.width = j.at("width").get<uint32_t>();
        x.x = j.at("x").get<int32_t>();
        x.y = j.at("y").get<int32_t>();
    }

    inline void to_json(json & j, const WinScreenRectangle & x) {
        j = json::object();
        j["bottom"] = x.bottom;
        j["height"] = x.height;
        j["left"] = x.left;
        j["right"] = x.right;
        j["top"] = x.top;
        j["width"] = x.width;
        j["x"] = x.x;
        j["y"] = x.y;
    }

    inline void from_json(const json & j, WinEdidInfo& x) {
        x.edid_bytes_base64 = get_stack_optional<std::string>(j, "edid_bytes_base64");
        x.manufacturer_vid = get_stack_optional<std::string>(j, "manufacturer_vid");
        x.max_horizontal_image_size_mm = get_stack_optional<double>(j, "max_horizontal_image_size_mm");
        x.max_vertical_image_size_mm = get_stack_optional<double>(j, "max_vertical_image_size_mm");
        x.monitor_device_path = j.at("monitor_device_path").get<std::string>();
        x.product_code_id = get_stack_optional<uint16_t>(j, "product_code_id");
        x.serial_number_id = get_stack_optional<uint32_t>(j, "serial_number_id");
        x.user_friendly_name = get_stack_optional<std::string>(j, "user_friendly_name");
        x.video_output_technology_type = get_stack_optional<WmiVideoOutputTechnology>(j, "video_output_technology_type");
        x.week_of_manufacture = get_stack_optional<uint8_t>(j, "week_of_manufacture");
        x.wmi_instance_name = get_stack_optional<std::string>(j, "wmi_instance_name");
        x.wmi_join_key = j.at("wmi_join_key").get<std::string>();
        x.year_of_manufacture = get_stack_optional<uint16_t>(j, "year_of_manufacture");
    }

    inline void to_json(json & j, const WinEdidInfo & x) {
        j = json::object();
        if (x.edid_bytes_base64) {
            j["edid_bytes_base64"] = x.edid_bytes_base64;
        }
        if (x.manufacturer_vid) {
            j["manufacturer_vid"] = x.manufacturer_vid;
        }
        if (x.max_horizontal_image_size_mm) {
            j["max_horizontal_image_size_mm"] = x.max_horizontal_image_size_mm;
        }
        if (x.max_vertical_image_size_mm) {
            j["max_vertical_image_size_mm"] = x.max_vertical_image_size_mm;
        }
        j["monitor_device_path"] = x.monitor_device_path;
        if (x.product_code_id) {
            j["product_code_id"] = x.product_code_id;
        }
        if (x.serial_number_id) {
            j["serial_number_id"] = x.serial_number_id;
        }
        if (x.user_friendly_name) {
            j["user_friendly_name"] = x.user_friendly_name;
        }
        if (x.video_output_technology_type) {
            j["video_output_technology_type"] = x.video_output_technology_type;
        }
        if (x.week_of_manufacture) {
            j["week_of_manufacture"] = x.week_of_manufacture;
        }
        if (x.wmi_instance_name) {
            j["wmi_instance_name"] = x.wmi_instance_name;
        }
        j["wmi_join_key"] = x.wmi_join_key;
        if (x.year_of_manufacture) {
            j["year_of_manufacture"] = x.year_of_manufacture;
        }
    }

    inline void from_json(const json & j, WinStandardColorInfo& x) {
        x.bits_per_channel = get_stack_optional<WinBitsPerColorChannel>(j, "bits_per_channel");
        x.color_encoding = get_stack_optional<WinColorEncoding>(j, "color_encoding");
        x.dxgi_color_space = get_stack_optional<WinDxgiColorSpace>(j, "dxgi_color_space");
        x.is_hdr_enabled = j.at("is_hdr_enabled").get<bool>();
        x.is_hdr_supported = j.at("is_hdr_supported").get<bool>();
        x.max_full_frame_luminance_nits = get_stack_optional<double>(j, "max_full_frame_luminance_nits");
        x.max_luminance_nits = get_stack_optional<double>(j, "max_luminance_nits");
        x.min_luminance_nits = get_stack_optional<double>(j, "min_luminance_nits");
    }

    inline void to_json(json & j, const WinStandardColorInfo & x) {
        j = json::object();
        if (x.bits_per_channel) {
            j["bits_per_channel"] = x.bits_per_channel;
        }
        if (x.color_encoding) {
            j["color_encoding"] = x.color_encoding;
        }
        if (x.dxgi_color_space) {
            j["dxgi_color_space"] = x.dxgi_color_space;
        }
        j["is_hdr_enabled"] = x.is_hdr_enabled;
        j["is_hdr_supported"] = x.is_hdr_supported;
        if (x.max_full_frame_luminance_nits) {
            j["max_full_frame_luminance_nits"] = x.max_full_frame_luminance_nits;
        }
        if (x.max_luminance_nits) {
            j["max_luminance_nits"] = x.max_luminance_nits;
        }
        if (x.min_luminance_nits) {
            j["min_luminance_nits"] = x.min_luminance_nits;
        }
    }

    inline void from_json(const json & j, WinDisplay& x) {
        x.adapter_device_path = get_stack_optional<std::string>(j, "adapter_device_path");
        x.adapter_instance_id = get_stack_optional<std::string>(j, "adapter_instance_id");
        x.advanced_color_info = get_stack_optional<WinAdvancedColorInfo>(j, "advanced_color_info");
        x.bounds = j.at("bounds").get<WinScreenRectangle>();
        x.dpi_scaling_percent = get_stack_optional<uint32_t>(j, "dpi_scaling_percent");
        x.edid_info = get_stack_optional<WinEdidInfo>(j, "edid_info");
        x.edid_key = get_stack_optional<std::string>(j, "edid_key");
        x.friendly_name = get_stack_optional<std::string>(j, "friendly_name");
        x.is_attached_to_desktop = get_stack_optional<bool>(j, "is_attached_to_desktop");
        x.is_primary = j.at("is_primary").get<bool>();
        x.monitor_device_path = get_stack_optional<std::string>(j, "monitor_device_path");
        x.monitor_path_key = get_stack_optional<std::string>(j, "monitor_path_key");
        x.physical_connector_type = get_stack_optional<WinDisplayConnectorType>(j, "physical_connector_type");
        x.primary_port_key = get_stack_optional<std::string>(j, "primary_port_key");
        x.refresh_rate_denominator = get_stack_optional<uint32_t>(j, "refresh_rate_denominator");
        x.refresh_rate_hz = get_stack_optional<double>(j, "refresh_rate_hz");
        x.refresh_rate_numerator = get_stack_optional<uint32_t>(j, "refresh_rate_numerator");
        x.rotation_deg = get_stack_optional<WinDisplayRotationDegrees>(j, "rotation_deg");
        x.scan_line_ordering = get_stack_optional<WinScanLineOrder>(j, "scan_line_ordering");
        x.short_lived_identifier = j.at("short_lived_identifier").get<std::string>();
        x.stable_id = get_stack_optional<std::string>(j, "stable_id");
        x.stable_id_candidates = get_stack_optional<std::vector<std::string>>(j, "stable_id_candidates");
        x.stable_id_source = get_stack_optional<StableIdSource>(j, "stable_id_source");
        x.standard_color_info = j.at("standard_color_info").get<WinStandardColorInfo>();
        x.target_path_id = get_stack_optional<uint32_t>(j, "target_path_id");
        x.working_area = j.at("working_area").get<WinScreenRectangle>();
    }

    inline void to_json(json & j, const WinDisplay & x) {
        j = json::object();
        if (x.adapter_device_path) {
            j["adapter_device_path"] = x.adapter_device_path;
        }
        if (x.adapter_instance_id) {
            j["adapter_instance_id"] = x.adapter_instance_id;
        }
        if (x.advanced_color_info) {
            j["advanced_color_info"] = x.advanced_color_info;
        }
        j["bounds"] = x.bounds;
        if (x.dpi_scaling_percent) {
            j["dpi_scaling_percent"] = x.dpi_scaling_percent;
        }
        if (x.edid_info) {
            j["edid_info"] = x.edid_info;
        }
        if (x.edid_key) {
            j["edid_key"] = x.edid_key;
        }
        if (x.friendly_name) {
            j["friendly_name"] = x.friendly_name;
        }
        if (x.is_attached_to_desktop) {
            j["is_attached_to_desktop"] = x.is_attached_to_desktop;
        }
        j["is_primary"] = x.is_primary;
        if (x.monitor_device_path) {
            j["monitor_device_path"] = x.monitor_device_path;
        }
        if (x.monitor_path_key) {
            j["monitor_path_key"] = x.monitor_path_key;
        }
        if (x.physical_connector_type) {
            j["physical_connector_type"] = x.physical_connector_type;
        }
        if (x.primary_port_key) {
            j["primary_port_key"] = x.primary_port_key;
        }
        if (x.refresh_rate_denominator) {
            j["refresh_rate_denominator"] = x.refresh_rate_denominator;
        }
        if (x.refresh_rate_hz) {
            j["refresh_rate_hz"] = x.refresh_rate_hz;
        }
        if (x.refresh_rate_numerator) {
            j["refresh_rate_numerator"] = x.refresh_rate_numerator;
        }
        if (x.rotation_deg) {
            j["rotation_deg"] = x.rotation_deg;
        }
        if (x.scan_line_ordering) {
            j["scan_line_ordering"] = x.scan_line_ordering;
        }
        j["short_lived_identifier"] = x.short_lived_identifier;
        if (x.stable_id) {
            j["stable_id"] = x.stable_id;
        }
        if (x.stable_id_candidates) {
            j["stable_id_candidates"] = x.stable_id_candidates;
        }
        if (x.stable_id_source) {
            j["stable_id_source"] = x.stable_id_source;
        }
        j["standard_color_info"] = x.standard_color_info;
        if (x.target_path_id) {
            j["target_path_id"] = x.target_path_id;
        }
        j["working_area"] = x.working_area;
    }

    inline void from_json(const json & j, WinDisplayProberJson& x) {
        x.displays = j.at("displays").get<std::vector<WinDisplay>>();
        x.has_interactive_desktop = j.at("has_interactive_desktop").get<bool>();
        x.is_remote_desktop = j.at("is_remote_desktop").get<bool>();
        x.is_virtual_machine = j.at("is_virtual_machine").get<bool>();
    }

    inline void to_json(json & j, const WinDisplayProberJson & x) {
        j = json::object();
        j["displays"] = x.displays;
        j["has_interactive_desktop"] = x.has_interactive_desktop;
        j["is_remote_desktop"] = x.is_remote_desktop;
        j["is_virtual_machine"] = x.is_virtual_machine;
    }

    inline void from_json(const json & j, WinDisplayRotationDegrees & x) {
        const auto value = j.get<int64_t>();
        switch (value) {
            case 0: x = WinDisplayRotationDegrees::VALUE_0; break;
            case 90: x = WinDisplayRotationDegrees::VALUE_90; break;
            case 180: x = WinDisplayRotationDegrees::VALUE_180; break;
            case 270: x = WinDisplayRotationDegrees::VALUE_270; break;
            default: throw std::runtime_error("Input JSON does not conform to schema!");
        }
    }

    inline void to_json(json & j, const WinDisplayRotationDegrees & x) {
        switch (x) {
            case WinDisplayRotationDegrees::VALUE_0: j = 0; break;
            case WinDisplayRotationDegrees::VALUE_90: j = 90; break;
            case WinDisplayRotationDegrees::VALUE_180: j = 180; break;
            case WinDisplayRotationDegrees::VALUE_270: j = 270; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"WinDisplayRotationDegrees\": " + std::to_string(static_cast<int64_t>(x)));
        }
    }

    inline void from_json(const json & j, WinBitsPerColorChannel & x) {
        const auto value = j.get<int64_t>();
        switch (value) {
            case 0: x = WinBitsPerColorChannel::VALUE_0; break;
            case 6: x = WinBitsPerColorChannel::VALUE_6; break;
            case 8: x = WinBitsPerColorChannel::VALUE_8; break;
            case 10: x = WinBitsPerColorChannel::VALUE_10; break;
            case 12: x = WinBitsPerColorChannel::VALUE_12; break;
            case 14: x = WinBitsPerColorChannel::VALUE_14; break;
            case 16: x = WinBitsPerColorChannel::VALUE_16; break;
            default: throw std::runtime_error("Input JSON does not conform to schema!");
        }
    }

    inline void to_json(json & j, const WinBitsPerColorChannel & x) {
        switch (x) {
            case WinBitsPerColorChannel::VALUE_0: j = 0; break;
            case WinBitsPerColorChannel::VALUE_6: j = 6; break;
            case WinBitsPerColorChannel::VALUE_8: j = 8; break;
            case WinBitsPerColorChannel::VALUE_10: j = 10; break;
            case WinBitsPerColorChannel::VALUE_12: j = 12; break;
            case WinBitsPerColorChannel::VALUE_14: j = 14; break;
            case WinBitsPerColorChannel::VALUE_16: j = 16; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"WinBitsPerColorChannel\": " + std::to_string(static_cast<int64_t>(x)));
        }
    }

    inline void from_json(const json & j, WmiVideoOutputTechnology & x) {
        const auto value = j.get<int64_t>();
        switch (value) {
            case -2: x = WmiVideoOutputTechnology::UNINITIALIZED; break;
            case -1: x = WmiVideoOutputTechnology::OTHER; break;
            case 0: x = WmiVideoOutputTechnology::VGA; break;
            case 1: x = WmiVideoOutputTechnology::SVIDEO; break;
            case 2: x = WmiVideoOutputTechnology::COMPOSITE_VIDEO; break;
            case 3: x = WmiVideoOutputTechnology::COMPONENT_VIDEO; break;
            case 4: x = WmiVideoOutputTechnology::DVI; break;
            case 5: x = WmiVideoOutputTechnology::HDMI; break;
            case 6: x = WmiVideoOutputTechnology::LVDS; break;
            case 8: x = WmiVideoOutputTechnology::D_JPN; break;
            case 9: x = WmiVideoOutputTechnology::SDI; break;
            case 10: x = WmiVideoOutputTechnology::DISPLAYPORT_EXTERNAL; break;
            case 11: x = WmiVideoOutputTechnology::DISPLAYPORT_EMBEDDED; break;
            case 12: x = WmiVideoOutputTechnology::UDI_EXTERNAL; break;
            case 13: x = WmiVideoOutputTechnology::UDI_EMBEDDED; break;
            case 14: x = WmiVideoOutputTechnology::SDTVDONGLE; break;
            case 15: x = WmiVideoOutputTechnology::MIRACAST; break;
            case 16: x = WmiVideoOutputTechnology::INDIRECT_WIRED; break;
            case 2147483648: x = WmiVideoOutputTechnology::INTERNAL; break;
            default: throw std::runtime_error("Input JSON does not conform to schema!");
        }
    }

    inline void to_json(json & j, const WmiVideoOutputTechnology & x) {
        switch (x) {
            case WmiVideoOutputTechnology::UNINITIALIZED: j = -2; break;
            case WmiVideoOutputTechnology::OTHER: j = -1; break;
            case WmiVideoOutputTechnology::VGA: j = 0; break;
            case WmiVideoOutputTechnology::SVIDEO: j = 1; break;
            case WmiVideoOutputTechnology::COMPOSITE_VIDEO: j = 2; break;
            case WmiVideoOutputTechnology::COMPONENT_VIDEO: j = 3; break;
            case WmiVideoOutputTechnology::DVI: j = 4; break;
            case WmiVideoOutputTechnology::HDMI: j = 5; break;
            case WmiVideoOutputTechnology::LVDS: j = 6; break;
            case WmiVideoOutputTechnology::D_JPN: j = 8; break;
            case WmiVideoOutputTechnology::SDI: j = 9; break;
            case WmiVideoOutputTechnology::DISPLAYPORT_EXTERNAL: j = 10; break;
            case WmiVideoOutputTechnology::DISPLAYPORT_EMBEDDED: j = 11; break;
            case WmiVideoOutputTechnology::UDI_EXTERNAL: j = 12; break;
            case WmiVideoOutputTechnology::UDI_EMBEDDED: j = 13; break;
            case WmiVideoOutputTechnology::SDTVDONGLE: j = 14; break;
            case WmiVideoOutputTechnology::MIRACAST: j = 15; break;
            case WmiVideoOutputTechnology::INDIRECT_WIRED: j = 16; break;
            case WmiVideoOutputTechnology::INTERNAL: j = 2147483648; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"WmiVideoOutputTechnology\": " + std::to_string(static_cast<int64_t>(x)));
        }
    }
    inline void from_json(const json & j, WinActiveColorMode & x) {
        if (j == "hdr") x = WinActiveColorMode::HDR;
        else if (j == "sdr") x = WinActiveColorMode::SDR;
        else if (j == "unspecified") x = WinActiveColorMode::UNSPECIFIED;
        else if (j == "wcg") x = WinActiveColorMode::WCG;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, const WinActiveColorMode & x) {
        switch (x) {
            case WinActiveColorMode::HDR: j = "hdr"; break;
            case WinActiveColorMode::SDR: j = "sdr"; break;
            case WinActiveColorMode::UNSPECIFIED: j = "unspecified"; break;
            case WinActiveColorMode::WCG: j = "wcg"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"WinActiveColorMode\": " + std::to_string(static_cast<int>(x)));
        }
    }

    inline void from_json(const json & j, WinDisplayConnectorType & x) {
        static std::unordered_map<std::string, WinDisplayConnectorType> enumValues {
            {"component_video", WinDisplayConnectorType::COMPONENT_VIDEO},
            {"composite_video", WinDisplayConnectorType::COMPOSITE_VIDEO},
            {"displayport_embedded", WinDisplayConnectorType::DISPLAYPORT_EMBEDDED},
            {"displayport_external", WinDisplayConnectorType::DISPLAYPORT_EXTERNAL},
            {"displayport_usb_tunnel", WinDisplayConnectorType::DISPLAYPORT_USB_TUNNEL},
            {"dvi", WinDisplayConnectorType::DVI},
            {"d_jpn", WinDisplayConnectorType::D_JPN},
            {"fake", WinDisplayConnectorType::FAKE},
            {"hdmi", WinDisplayConnectorType::HDMI},
            {"indirect_virtual", WinDisplayConnectorType::INDIRECT_VIRTUAL},
            {"indirect_wired", WinDisplayConnectorType::INDIRECT_WIRED},
            {"internal", WinDisplayConnectorType::INTERNAL},
            {"lvds", WinDisplayConnectorType::LVDS},
            {"miracast", WinDisplayConnectorType::MIRACAST},
            {"other", WinDisplayConnectorType::OTHER},
            {"rdp", WinDisplayConnectorType::RDP},
            {"sdi", WinDisplayConnectorType::SDI},
            {"sdtvdongle", WinDisplayConnectorType::SDTVDONGLE},
            {"svideo", WinDisplayConnectorType::SVIDEO},
            {"udi_embedded", WinDisplayConnectorType::UDI_EMBEDDED},
            {"udi_external", WinDisplayConnectorType::UDI_EXTERNAL},
            {"vga", WinDisplayConnectorType::VGA},
        };
        auto iter = enumValues.find(j.get<std::string>());
        if (iter != enumValues.end()) {
            x = iter->second;
        }
    }

    inline void to_json(json & j, const WinDisplayConnectorType & x) {
        switch (x) {
            case WinDisplayConnectorType::COMPONENT_VIDEO: j = "component_video"; break;
            case WinDisplayConnectorType::COMPOSITE_VIDEO: j = "composite_video"; break;
            case WinDisplayConnectorType::DISPLAYPORT_EMBEDDED: j = "displayport_embedded"; break;
            case WinDisplayConnectorType::DISPLAYPORT_EXTERNAL: j = "displayport_external"; break;
            case WinDisplayConnectorType::DISPLAYPORT_USB_TUNNEL: j = "displayport_usb_tunnel"; break;
            case WinDisplayConnectorType::DVI: j = "dvi"; break;
            case WinDisplayConnectorType::D_JPN: j = "d_jpn"; break;
            case WinDisplayConnectorType::FAKE: j = "fake"; break;
            case WinDisplayConnectorType::HDMI: j = "hdmi"; break;
            case WinDisplayConnectorType::INDIRECT_VIRTUAL: j = "indirect_virtual"; break;
            case WinDisplayConnectorType::INDIRECT_WIRED: j = "indirect_wired"; break;
            case WinDisplayConnectorType::INTERNAL: j = "internal"; break;
            case WinDisplayConnectorType::LVDS: j = "lvds"; break;
            case WinDisplayConnectorType::MIRACAST: j = "miracast"; break;
            case WinDisplayConnectorType::OTHER: j = "other"; break;
            case WinDisplayConnectorType::RDP: j = "rdp"; break;
            case WinDisplayConnectorType::SDI: j = "sdi"; break;
            case WinDisplayConnectorType::SDTVDONGLE: j = "sdtvdongle"; break;
            case WinDisplayConnectorType::SVIDEO: j = "svideo"; break;
            case WinDisplayConnectorType::UDI_EMBEDDED: j = "udi_embedded"; break;
            case WinDisplayConnectorType::UDI_EXTERNAL: j = "udi_external"; break;
            case WinDisplayConnectorType::VGA: j = "vga"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"WinDisplayConnectorType\": " + std::to_string(static_cast<int>(x)));
        }
    }

    inline void from_json(const json & j, WinScanLineOrder & x) {
        if (j == "interlaced_lower_field_first") x = WinScanLineOrder::INTERLACED_LOWER_FIELD_FIRST;
        else if (j == "interlaced_upper_field_first") x = WinScanLineOrder::INTERLACED_UPPER_FIELD_FIRST;
        else if (j == "progressive") x = WinScanLineOrder::PROGRESSIVE;
        else if (j == "unspecified") x = WinScanLineOrder::UNSPECIFIED;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, const WinScanLineOrder & x) {
        switch (x) {
            case WinScanLineOrder::INTERLACED_LOWER_FIELD_FIRST: j = "interlaced_lower_field_first"; break;
            case WinScanLineOrder::INTERLACED_UPPER_FIELD_FIRST: j = "interlaced_upper_field_first"; break;
            case WinScanLineOrder::PROGRESSIVE: j = "progressive"; break;
            case WinScanLineOrder::UNSPECIFIED: j = "unspecified"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"WinScanLineOrder\": " + std::to_string(static_cast<int>(x)));
        }
    }

    inline void from_json(const json & j, StableIdSource & x) {
        if (j == "edid_key") x = StableIdSource::EDID_KEY;
        else if (j == "monitor_path_key") x = StableIdSource::MONITOR_PATH_KEY;
        else if (j == "primary_port_key") x = StableIdSource::PRIMARY_PORT_KEY;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, const StableIdSource & x) {
        switch (x) {
            case StableIdSource::EDID_KEY: j = "edid_key"; break;
            case StableIdSource::MONITOR_PATH_KEY: j = "monitor_path_key"; break;
            case StableIdSource::PRIMARY_PORT_KEY: j = "primary_port_key"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"StableIdSource\": " + std::to_string(static_cast<int>(x)));
        }
    }

    inline void from_json(const json & j, WinColorEncoding & x) {
        if (j == "rgb") x = WinColorEncoding::RGB;
        else if (j == "unspecified") x = WinColorEncoding::UNSPECIFIED;
        else if (j == "ycbcr420") x = WinColorEncoding::YCBCR420;
        else if (j == "ycbcr422") x = WinColorEncoding::YCBCR422;
        else if (j == "ycbcr444") x = WinColorEncoding::YCBCR444;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, const WinColorEncoding & x) {
        switch (x) {
            case WinColorEncoding::RGB: j = "rgb"; break;
            case WinColorEncoding::UNSPECIFIED: j = "unspecified"; break;
            case WinColorEncoding::YCBCR420: j = "ycbcr420"; break;
            case WinColorEncoding::YCBCR422: j = "ycbcr422"; break;
            case WinColorEncoding::YCBCR444: j = "ycbcr444"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"WinColorEncoding\": " + std::to_string(static_cast<int>(x)));
        }
    }

    inline void from_json(const json & j, WinDxgiColorSpace & x) {
        static std::unordered_map<std::string, WinDxgiColorSpace> enumValues {
            {"custom", WinDxgiColorSpace::CUSTOM},
            {"reserved", WinDxgiColorSpace::RESERVED},
            {"rgb_full_g10_none_p709", WinDxgiColorSpace::RGB_FULL_G10_NONE_P709},
            {"rgb_full_g2084_none_p2020", WinDxgiColorSpace::RGB_FULL_G2084_NONE_P2020},
            {"rgb_full_g22_none_p2020", WinDxgiColorSpace::RGB_FULL_G22_NONE_P2020},
            {"rgb_full_g22_none_p709", WinDxgiColorSpace::RGB_FULL_G22_NONE_P709},
            {"rgb_studio_g2084_none_p2020", WinDxgiColorSpace::RGB_STUDIO_G2084_NONE_P2020},
            {"rgb_studio_g22_none_p2020", WinDxgiColorSpace::RGB_STUDIO_G22_NONE_P2020},
            {"rgb_studio_g22_none_p709", WinDxgiColorSpace::RGB_STUDIO_G22_NONE_P709},
            {"rgb_studio_g24_none_p2020", WinDxgiColorSpace::RGB_STUDIO_G24_NONE_P2020},
            {"rgb_studio_g24_none_p709", WinDxgiColorSpace::RGB_STUDIO_G24_NONE_P709},
            {"ycbcr_full_g22_left_p2020", WinDxgiColorSpace::YCBCR_FULL_G22_LEFT_P2020},
            {"ycbcr_full_g22_left_p601", WinDxgiColorSpace::YCBCR_FULL_G22_LEFT_P601},
            {"ycbcr_full_g22_left_p709", WinDxgiColorSpace::YCBCR_FULL_G22_LEFT_P709},
            {"ycbcr_full_g22_none_p709_x601", WinDxgiColorSpace::YCBCR_FULL_G22_NONE_P709_X601},
            {"ycbcr_full_ghlg_topleft_p2020", WinDxgiColorSpace::YCBCR_FULL_GHLG_TOPLEFT_P2020},
            {"ycbcr_studio_g2084_left_p2020", WinDxgiColorSpace::YCBCR_STUDIO_G2084_LEFT_P2020},
            {"ycbcr_studio_g2084_topleft_p2020", WinDxgiColorSpace::YCBCR_STUDIO_G2084_TOPLEFT_P2020},
            {"ycbcr_studio_g22_left_p2020", WinDxgiColorSpace::YCBCR_STUDIO_G22_LEFT_P2020},
            {"ycbcr_studio_g22_left_p601", WinDxgiColorSpace::YCBCR_STUDIO_G22_LEFT_P601},
            {"ycbcr_studio_g22_left_p709", WinDxgiColorSpace::YCBCR_STUDIO_G22_LEFT_P709},
            {"ycbcr_studio_g22_topleft_p2020", WinDxgiColorSpace::YCBCR_STUDIO_G22_TOPLEFT_P2020},
            {"ycbcr_studio_g24_left_p2020", WinDxgiColorSpace::YCBCR_STUDIO_G24_LEFT_P2020},
            {"ycbcr_studio_g24_left_p709", WinDxgiColorSpace::YCBCR_STUDIO_G24_LEFT_P709},
            {"ycbcr_studio_g24_topleft_p2020", WinDxgiColorSpace::YCBCR_STUDIO_G24_TOPLEFT_P2020},
            {"ycbcr_studio_ghlg_topleft_p2020", WinDxgiColorSpace::YCBCR_STUDIO_GHLG_TOPLEFT_P2020},
        };
        auto iter = enumValues.find(j.get<std::string>());
        if (iter != enumValues.end()) {
            x = iter->second;
        }
    }

    inline void to_json(json & j, const WinDxgiColorSpace & x) {
        switch (x) {
            case WinDxgiColorSpace::CUSTOM: j = "custom"; break;
            case WinDxgiColorSpace::RESERVED: j = "reserved"; break;
            case WinDxgiColorSpace::RGB_FULL_G10_NONE_P709: j = "rgb_full_g10_none_p709"; break;
            case WinDxgiColorSpace::RGB_FULL_G2084_NONE_P2020: j = "rgb_full_g2084_none_p2020"; break;
            case WinDxgiColorSpace::RGB_FULL_G22_NONE_P2020: j = "rgb_full_g22_none_p2020"; break;
            case WinDxgiColorSpace::RGB_FULL_G22_NONE_P709: j = "rgb_full_g22_none_p709"; break;
            case WinDxgiColorSpace::RGB_STUDIO_G2084_NONE_P2020: j = "rgb_studio_g2084_none_p2020"; break;
            case WinDxgiColorSpace::RGB_STUDIO_G22_NONE_P2020: j = "rgb_studio_g22_none_p2020"; break;
            case WinDxgiColorSpace::RGB_STUDIO_G22_NONE_P709: j = "rgb_studio_g22_none_p709"; break;
            case WinDxgiColorSpace::RGB_STUDIO_G24_NONE_P2020: j = "rgb_studio_g24_none_p2020"; break;
            case WinDxgiColorSpace::RGB_STUDIO_G24_NONE_P709: j = "rgb_studio_g24_none_p709"; break;
            case WinDxgiColorSpace::YCBCR_FULL_G22_LEFT_P2020: j = "ycbcr_full_g22_left_p2020"; break;
            case WinDxgiColorSpace::YCBCR_FULL_G22_LEFT_P601: j = "ycbcr_full_g22_left_p601"; break;
            case WinDxgiColorSpace::YCBCR_FULL_G22_LEFT_P709: j = "ycbcr_full_g22_left_p709"; break;
            case WinDxgiColorSpace::YCBCR_FULL_G22_NONE_P709_X601: j = "ycbcr_full_g22_none_p709_x601"; break;
            case WinDxgiColorSpace::YCBCR_FULL_GHLG_TOPLEFT_P2020: j = "ycbcr_full_ghlg_topleft_p2020"; break;
            case WinDxgiColorSpace::YCBCR_STUDIO_G2084_LEFT_P2020: j = "ycbcr_studio_g2084_left_p2020"; break;
            case WinDxgiColorSpace::YCBCR_STUDIO_G2084_TOPLEFT_P2020: j = "ycbcr_studio_g2084_topleft_p2020"; break;
            case WinDxgiColorSpace::YCBCR_STUDIO_G22_LEFT_P2020: j = "ycbcr_studio_g22_left_p2020"; break;
            case WinDxgiColorSpace::YCBCR_STUDIO_G22_LEFT_P601: j = "ycbcr_studio_g22_left_p601"; break;
            case WinDxgiColorSpace::YCBCR_STUDIO_G22_LEFT_P709: j = "ycbcr_studio_g22_left_p709"; break;
            case WinDxgiColorSpace::YCBCR_STUDIO_G22_TOPLEFT_P2020: j = "ycbcr_studio_g22_topleft_p2020"; break;
            case WinDxgiColorSpace::YCBCR_STUDIO_G24_LEFT_P2020: j = "ycbcr_studio_g24_left_p2020"; break;
            case WinDxgiColorSpace::YCBCR_STUDIO_G24_LEFT_P709: j = "ycbcr_studio_g24_left_p709"; break;
            case WinDxgiColorSpace::YCBCR_STUDIO_G24_TOPLEFT_P2020: j = "ycbcr_studio_g24_topleft_p2020"; break;
            case WinDxgiColorSpace::YCBCR_STUDIO_GHLG_TOPLEFT_P2020: j = "ycbcr_studio_ghlg_topleft_p2020"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"WinDxgiColorSpace\": " + std::to_string(static_cast<int>(x)));
        }
    }
}
