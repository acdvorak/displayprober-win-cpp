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

    struct WinSetupApiDevice {
        /**
         * Example:
         *
         * - `0`
         * - `273`
         * - `275`
         */
        std::optional<uint32_t> address;
        /**
         * Examples:
         *
         * - `"{00000000-0000-0000-FFFF-FFFFFFFFFFFF}"`
         * - `"{840576F3-7055-5A87-9859-A63E0FA9F3DA}"`
         * - `"{DA0631A1-C6DF-5655-BEA1-9110DBA37845}"`
         */
        std::optional<std::string> base_container_id;
        /**
         * Example:
         *
         * - `0`
         * - `5`
         */
        std::optional<uint32_t> bus_number;
        /**
         * Example:
         *
         * - `"{C6CA0D74-E43B-4ABD-A63A-4BD2AD319D60}"`
         * - `"{C8EBDFB0-B510-11D0-80E5-00A0C92542E3}"`
         */
        std::optional<std::string> bus_type_guid;
        /**
         * Examples:
         *
         * - `0`
         * - `228`
         */
        std::optional<uint32_t> capabilities;
        std::optional<uint32_t> characteristics;
        /**
         * Examples:
         *
         * - `"{4D36E968-E325-11CE-BFC1-08002BE10318}"` (Display)
         * - `"{4D36E96E-E325-11CE-BFC1-08002BE10318}"` (Monitor)
         * - `"{4D36E97D-E325-11CE-BFC1-08002BE10318}"` (System)
         */
        std::optional<std::string> class_guid;
        /**
         * Examples:
         *
         * - `"Display"`
         * - `"Monitor"`
         * - `"System"`
         */
        std::optional<std::string> class_name;
        /**
         * Examples:
         *
         * ```jsonc [   "PCI\\VEN_10DE&DEV_2584&REV_A1",   "PCI\\VEN_10DE&DEV_2584",
         * "PCI\\VEN_10DE&CC_030000",   "PCI\\VEN_10DE&CC_0300",   "PCI\\VEN_10DE",
         * "PCI\\CC_030000",   "PCI\\CC_0300", ] ```
         *
         * ```jsonc [   "*PNP09FF" ] ```
         */
        std::optional<std::vector<std::string>> compatible_ids;
        /**
         * Examples:
         *
         * - `0`
         */
        std::optional<uint32_t> config_flags;
        std::optional<uint32_t> dev_type;
        /**
         * Examples:
         *
         * - `"Generic PnP Monitor"`
         * - `"Microsoft Basic Display Driver"`
         * - `"NVIDIA GeForce RTX 3050"`
         */
        std::optional<std::string> device_desc;
        /**
         * ⚠️ Opaque device path. Use for case-insensitive string comparisons with other APIs.
         *
         * Data source: `SP_DEVICE_INTERFACE_DETAIL_DATA_W.DevicePath`
         *
         * Usage:
         *
         * - ✅ Use it as a join key to correlate data from different APIs.
         * - ✅ ALWAYS use case-insensitive string comparisons.
         * - ❌ Do NOT assume it is always lowercase.
         * - ❌ Do NOT parse the value.
         *
         * According to Microsoft, there is no API contract that the string will _always_ be
         * lowercase, though that is what I have observed in practice across Windows XP through
         * Windows 11.
         *
         * [Device identification
         * strings](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/device-identification-strings):
         *
         * > Device identification strings **should not be parsed**. They are meant > only for
         * string comparisons and should be treated as **opaque strings**.
         *
         * [`SetupDiGetDeviceInterfaceDetailW()
         * docs`](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetailw#remarks):
         *
         * > **Do not attempt to parse the device path symbolic name.** > > The device path can be
         * reused across system starts.
         *
         * [`IoGetDeviceInterfaces()`
         * docs](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-iogetdeviceinterfaces#remarks):
         *
         * > **The format of a symbolic link name is opaque; the caller should not > attempt to
         * parse a symbolic link name.** > > Symbolic links for device interface instances can be
         * used across system > boots.
         *
         * [`IoRegisterDeviceInterface()`
         * docs](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-ioregisterdeviceinterface#parameters):
         *
         * > `SymbolicLinkName`: **kernel-mode path to the symbolic link** for an > instance of the
         * specified device interface class. > > **The caller must treat `SymbolicLinkName` as
         * opaque** and **must not** > disassemble it.
         *
         * Examples:
         *
         * - `"\\\\?\\display#sam73a5#5&757fe5e&7&uid20737#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
         * - `"\\\\?\\display#viz1009#5&757fe5e&7&uid20739#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
         * -
         * `"\\\\?\\pci#ven_10de&dev_2584&subsys_184610de&rev_a1#4&2b1c6285&0&0010#{5b45201d-f2f2-4f3b-85bb-30ff1f953599}"`
         * - `"\\\\?\\root#basicdisplay#0000#{5b45201d-f2f2-4f3b-85bb-30ff1f953599}"`
         */
        std::string device_path_mixed_case;
        /**
         * Examples:
         *
         * - `"{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"`
         * - `"{4d36e96e-e325-11ce-bfc1-08002be10318}\\0004"`
         * - `"{4d36e96e-e325-11ce-bfc1-08002be10318}\\0005"`
         * - `"{4d36e97d-e325-11ce-bfc1-08002be10318}\\0051"`
         */
        std::optional<std::string> driver;
        /**
         * Example:
         *
         * - `"DISPLAY"`
         * - `"PCI"`
         * - `"ROOT"`
         */
        std::optional<std::string> enumerator_name;
        /**
         * Examples:
         *
         * - `"Generic Monitor (E390-B0)"`
         * - `"Generic Monitor (QCQ95S)"`
         */
        std::optional<std::string> friendly_name;
        /**
         * Examples:
         *
         * ```jsonc [   "PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1",
         * "PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE",   "PCI\\VEN_10DE&DEV_2584&CC_030000",
         * "PCI\\VEN_10DE&DEV_2584&CC_0300" ] ```
         *
         * ```jsonc [   "ROOT\\BasicDisplay" ] ```
         *
         * ```jsonc [   "MONITOR\\SAM73A5" ] ```
         *
         * ```jsonc [   "MONITOR\\VIZ1009" ] ```
         */
        std::optional<std::vector<std::string>> hardware_id;
        /**
         * Data source: `SetupDiGetDeviceInstanceIdW()`
         *
         * Examples:
         *
         * - `"DISPLAY\\SAM73A5\\5&757FE5E&7&UID20737"`
         * - `"DISPLAY\\VIZ1009\\5&757FE5E&7&UID20739"`
         * - `"PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1\\4&2B1C6285&0&0010"`
         * - `"ROOT\\BASICDISPLAY\\0000"`
         */
        std::optional<std::string> instance_id;
        /**
         * Example:
         *
         * - `5`
         * - `15`
         */
        std::optional<uint32_t> legacy_bus_type;
        /**
         * Examples:
         *
         * - `"PCI bus 5, device 0, function 0"`
         */
        std::optional<std::string> location_information;
        /**
         * Examples:
         *
         * ```jsonc [   "PCIROOT(0)#PCI(0200)#PCI(0000)",
         * "ACPI(_SB_)#ACPI(PCI0)#ACPI(NPE2)#ACPI(SLT2)", ] ```
         */
        std::optional<std::vector<std::string>> location_paths;
        /**
         * Examples:
         *
         * - `"(Standard display types)"`
         * - `"(Standard monitor types)"`
         * - `"NVIDIA"`
         */
        std::optional<std::string> mfg;
        /**
         * Examples:
         *
         * - `"\\Device\\00000003"`
         * - `"\\Device\\0000006d"`
         * - `"\\Device\\0000006e"`
         * - `"\\Device\\NTPNP_PCI0024"`
         */
        std::optional<std::string> physical_device_object_name;
        /**
         * Examples:
         *
         * - `"nvlddmkm"`
         * - `"BasicDisplay"`
         * - `"monitor"`
         */
        std::optional<std::string> service;
        /**
         * Example:
         *
         * - `2`
         */
        std::optional<uint32_t> ui_number;
        std::optional<std::string> ui_number_desc_format;
    };

    struct WinSetupApiDeviceCatalog {
        std::vector<WinSetupApiDevice> adapters;
        std::vector<WinSetupApiDevice> monitors;
    };

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

    /**
     * An individual monitor or virtual display attached to a Windows PC.
     */
    struct WinDisplay {
        /**
         * Effectively a reformatted version of  {@link  adapter_instance_id } , plus a static,
         * hard-coded device interface class GUID for adapters
         * (`GUID_DEVINTERFACE_DISPLAY_ADAPTER`).
         *
         * Source: `DISPLAYCONFIG_ADAPTER_NAME.adapterDevicePath` in `wingdi.h`.
         *
         * Example:
         *
         * -
         * `"\\\\?\\PCI#VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1#4&2b1c6285&0&0010#{5b45201d-f2f2-4f3b-85bb-30ff1f953599}"`
         *
         * Characteristics:
         *
         * - ✅ Stable and persistent across reboots.
         * - ❓ Unclear if persistent across driver upgrades.
         *
         * The GUID at the end is a fixed, Microsoft-defined class GUID (declared in `Ntddvdeo.h`),
         * and Windows appends it as part of the device-interface symbolic link format when the
         * display stack registers that interface.
         *
         * See:
         *
         * - [`GUID_DEVINTERFACE_DISPLAY_ADAPTER` constant
         * (`Ntddvdeo.h`)](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/guid-devinterface-display-adapter)
         * - [`DISPLAYCONFIG_ADAPTER_NAME` structure
         * (`wingdi.h`)](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_adapter_name)
         * - [`SetupDiGetDeviceInterfaceDetailA()` function
         * (`setupapi.h`)](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetaila)
         * - [`IoRegisterDeviceInterface()` function
         * (`wdm.h`)](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-ioregisterdeviceinterface)
         */
        std::optional<std::string> adapter_device_path;
        /**
         * Human-friendly name of the adapter (typically the GPU).
         *
         * Source: `DISPLAY_DEVICEW.DeviceString` in `wingdi.h`.
         *
         * Example:
         *
         * - `"NVIDIA GeForce RTX 3050"`
         */
        std::optional<std::string> adapter_friendly_name;
        /**
         * Hardware identifier of the adapter (typically the GPU).
         *
         * Source: `DISPLAY_DEVICEW.DeviceID` in `wingdi.h`.
         *
         * Example:
         *
         * - `"PCI\\VEN_10DE&DEV_2584&SUBSYS_184610DE&REV_A1"`
         *
         * Characteristics:
         *
         * - ✅ Static physical hardware identifier.
         * - ✅ Stable and persistent across reboots and driver upgrades.
         * - ⚠️ Shared by all PORTS on a physical discrete GPU.
         * - ⚠️ Shared if you have multiple identical GPUs.
         *
         * For physical discrete GPUs, this value is the PCI ID. See:
         *
         * -
         * https://learn.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-pci-devices
         * - https://pci-ids.ucw.cz/
         *
         * Format:
         *
         * ``` PCI \ VEN_10DE & DEV_2584 & SUBSYS_184610DE & REV_A1       ┗━━━┳━━┛   ┗━━━┳━━┛
         * ┗━━━━━━┳━━━━━━┛   ┗━━┳━┛        Vendor     Device       Subsystem     Revision ```
         *
         * The above example is an NVIDIA GeForce RTX 3050 6GB PCIe graphics card:
         *
         * https://pcilookup.com/?ven=10DE&dev=2584&action=submit
         */
        std::optional<std::string> adapter_hardware_id;
        /**
         * Unique Plug-n-Play instance ID of the *individual GPU*, equal to  {@link
         * adapter_hardware_id }  + serial/location (PCI slot number).
         *
         * Source: `SetupDiGetDeviceInstanceIdW()` in `SetupAPI.h`.
         *
         * Example:
         *
         * - `"PCI\\VEN_10DE&DEV_0DF8&SUBSYS_083510DE&REV_A1\\4&2B1C6285&0&0010"`
         *
         * Characteristics:
         *
         * - ✅ Static physical hardware ID + serial + location (PCI slot number).
         * - ✅ Stable and persistent across reboots and driver upgrades.
         * - ⚠️ Shared by all PORTS on a physical discrete GPU.
         * - ✅ Unique per physical GPU instance.
         *
         * If your PC has two identical GPUs, each one will have its own unique
         * `adapter_instance_id` value.
         *
         * For a single GPU with multiple ports (e.g., one DisplayPort and one DVI), all connected
         * displays will have the same `adapter_instance_id` value.
         *
         * ### References
         *
         * From [Windows Drivers > Device Instance
         * ID](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/device-instance-ids):
         *
         * > A device instance ID is a system-supplied device identification string > that uniquely
         * identifies a device in the system. The Plug and Play (PnP) > manager assigns a device
         * instance ID to each device node (devnode) in a > system's device tree. > > The creation
         * of the device instance ID for a device uses the bus driver > reported device ID value,
         * instance ID value, and the UniqueID member of > the DEVICE_CAPABILITIES structure as
         * input in order to create the unique > device instance ID for this device on the system. >
         * > ``` > PCI\VEN_1000&DEV_0001&SUBSYS_00000000&REV_02\1&08 > ```
         */
        std::optional<std::string> adapter_instance_id;
        /**
         * Per-GPU-port registry key.
         *
         * Source: `DISPLAY_DEVICEW.DeviceKey` in `wingdi.h`.
         *
         * Examples:
         *
         * -
         * `"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video\\{46D2BE53-1822-11F1-85AE-806E6F6E6963}\\0000"`
         * -
         * `"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video\\{46D2BE53-1822-11F1-85AE-806E6F6E6963}\\0001"`
         *
         * Characteristics:
         *
         * - ❓ TODO(acdvorak): Describe uniqueness, persistence, and stability across   reboots,
         * device disconnects/reconnects, and port/dock changes.
         *
         * The GUID is a local, OS-generated adapter/video-stack identifier, likely the adapter
         * `VideoID` created by the video port / display driver machinery.
         *
         * The numeric suffix is an auto-incrementing "child adapter index" for the physical port on
         * the GPU (in the typical case).
         *
         * ### References
         *
         * According to [WebRTC
         * `win/screen_capture_utils.cc`](https://webrtc.googlesource.com/src/+/c0fd2e0/modules/desktop_capture/win/screen_capture_utils.cc?pli=1#184):
         *
         * > `DeviceKey` is documented as reserved, but it actually contains the > registry key for
         * the device and is unique for each monitor, while > `DeviceID` is not.
         *
         * According to [ReactOS Display Driver
         * Loading](https://reactos.org/wiki/Techwiki:Win32k/display_driver_loading), the format of
         * this key is:
         *
         * > Device configuration key: >
         * `"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video\\<VideoId>\\0000"` >
         * where `<VideoId>` is a local UUID, created by `videoprt` the first time > the device is
         * started. The `VideoId` string is stored in the registry > under the device's hardware key
         * > (`HKLM\System\CurrentControlSet\Enum\...`).
         */
        std::optional<std::string> adapter_registry_key;
        std::optional<WinAdvancedColorInfo> advanced_color_info;
        /**
         * The full size and position of the display, *including* the taskbar and any other areas
         * that are not usable by maximized (non-fullscreen) applications.
         */
        WinScreenRectangle bounds;
        /**
         * Typical values allowed by the Windows Display Settings UI are:
         *
         * `100 | 125 | 150 | 175 | 200 | 225 | 250 | 275 | 300 | ... | 500`
         *
         * Technically, the user can set any arbitrary value they want via registry hacks.
         */
        std::optional<uint32_t> dpi_scaling_percent;
        std::optional<WinEdidInfo> edid_info;
        /**
         * ✅ TERTIARY STABLE ID (when available)
         *
         * Deterministic monitor identity key based on EDID.
         *
         * Only emitted when manufacturer, product code, and serial are available.
         *
         * TODO(acdvorak): Append a hash of the full EDID bytes.
         */
        std::optional<std::string> edid_key;
        /**
         * Human-friendly name of the display.
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
         *
         * Value comes from one of the following sources, in descending order of quality (i.e., the
         * "best" available value is returned):
         *
         * 1. EDID "monitor descriptor" name (e.g., `"DELL ST2320L"`) 2. `"Remote Desktop"` or
         * `"Remote Desktop #N"` if RDP 3. `"Virtual Machine"` or `"Virtual Machine #N"` if a VM 4.
         * 7-digit Windows EDID identifier (e.g., `"SAM73A5"` or `"DELF023"`) 5. Short-lived Windows
         * display number (e.g., `"DISPLAY1"`)
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
         * connection path. Useful for correlating to EDID retrieval.
         *
         * Source: `DISPLAYCONFIG_TARGET_DEVICE_NAME.monitorDevicePath` in `wingdi.h`.
         *
         * Examples:
         *
         * -
         * `"\\\\?\\DISPLAY#SAM7346#5&21e6c3e1&0&UID5243153#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
         * -
         * `"\\\\?\\DISPLAY#DELF023#5&21e6c3e1&0&UID5243152#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}"`
         *
         * Characteristics:
         *
         * - ❓ TODO(acdvorak): Describe stability/uniqueness
         * - ❓ TODO(acdvorak): What is the GUID?
         */
        std::optional<std::string> monitor_device_path;
        /**
         * Examples:
         *
         * - `"{4d36e96e-e325-11ce-bfc1-08002be10318}\\0004"`
         * - `"{4d36e96e-e325-11ce-bfc1-08002be10318}\\0005"`
         *
         * The GUID `{4d36e96e-e325-11ce-bfc1-08002be10318}` is `GUID_DEVCLASS_MONITOR`, which is
         * the system-defined setup class for monitors.
         *
         * The last 4 numeric digits (like `0004` or `0005`) represent a zero-padded
         * *instance identifier* (often called the *driver node index*) assigned sequentially by the
         * Windows Plug and Play (PnP) manager.
         *
         * Specifically, this 4-digit number acts as a pointer to the *Driver Key* (also known as
         * the *Software Key*) in the Windows Registry where the operating system stores the driver
         * and configuration parameters for that exact monitor.
         *
         * When a monitor is connected, Windows looks at the Device Setup Class GUID (the
         * `{4d36e96e-e325-11ce-bfc1-08002be10318}` part, which dictates that the device is a
         * "Monitor") and assigns it the next available 4-digit number starting from `0000`.
         *
         * This means the `0004` in the first example above maps directly to this specific registry
         * path, specified by  {@link  monitor_registry_key } :
         *
         * ```
         * HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e96e-e325-11ce-bfc1-08002be10318}\0004
         * ```
         *
         * If you navigate to that specific subkey in the Registry Editor, you will find
         * software-level properties for that monitor instance.
         */
        std::optional<std::string> monitor_driver_key;
        /**
         * Examples:
         *
         * - `"DISPLAY\\SAM73A5\\5&757FE5E&6&UID20737"`
         * - `"DISPLAY\\VIZ1009\\5&757FE5E&6&UID20739"`
         */
        std::optional<std::string> monitor_instance_id;
        /**
         * ✅ SECONDARY STABLE ID (when available)
         *
         * Deterministic key derived from  {@link  monitor_device_path } .
         */
        std::optional<std::string> monitor_path_key;
        /**
         * Examples:
         *
         * -
         * `"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e96e-e325-11ce-bfc1-08002be10318}\\0004"`
         * -
         * `"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e96e-e325-11ce-bfc1-08002be10318}\\0005"`
         *
         * Common values stored here include:
         *
         * - **DriverDesc**: The friendly, human-readable name of the monitor   (e.g., "Generic PnP
         * Monitor", "Generic Non-PnP Monitor", or   "Samsung SyncMaster").
         *
         * - **MatchingDeviceId**: The PnP hardware ID used to match the driver to the   monitor
         * (e.g., "*PNP09FF" or "MONITOR\Default_Monitor").
         *
         * - **ProviderName**: The author of the driver (usually "Microsoft" for   standard Plug and
         * Play monitors).
         *
         * - **EDID Overrides**: Any manual software overrides applied to the   monitor's Extended
         * Display Identification Data (EDID).
         */
        std::optional<std::string> monitor_registry_key;
        /**
         * Maybe EDID-derived?
         *
         * Example:
         *
         * - `"LAU8PSBP01000"` (Vizio TV)
         *
         * TODO(acdvorak): Figure out which Windows API returns this value. NirSoft MultiMonitorTool
         * knows how to get it.
         */
        std::optional<std::string> monitor_serial_string;
        /**
         * Examples:
         *
         * - `"Generic PnP Monitor"`
         * - `"Generic Non-PnP Monitor"`
         *
         * TODO(acdvorak): Get this value from the registry (`DriverDesc`). See  {@link
         * monitor_registry_key } .
         */
        std::optional<std::string> monitor_string;
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
        std::vector<WinSetupApiDeviceCatalog> setup_api_devices;
        /**
         * Windows "monitor device name".
         *
         * Source: `MONITORINFOEXW.szDevice` via `GetMonitorInfoW()` in `WinUser.h`.
         *
         * Characteristics:
         *
         * - ⚠️ NOT stable across device disconnects/reconnects.
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
         * Per-adapter identifier of the target display endpoint used by `DisplayConfig` APIs to
         * address/query a specific path target.
         *
         * Source: `DISPLAYCONFIG_PATH_INFO.targetInfo.id` in `wingdi.h`.
         */
        std::optional<uint32_t> target_path_id;
        /**
         * Available working area on the screen, *excluding* taskbars and other docked windows.
         */
        WinScreenRectangle working_area;
    };

    struct WinDisplayProberJson {
        WinSetupApiDeviceCatalog all_setup_api_devices;
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
    void from_json(const json & j, WinSetupApiDevice & x);
    void to_json(json & j, const WinSetupApiDevice & x);

    void from_json(const json & j, WinSetupApiDeviceCatalog & x);
    void to_json(json & j, const WinSetupApiDeviceCatalog & x);

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

    inline void from_json(const json & j, WinSetupApiDevice& x) {
        x.address = get_stack_optional<uint32_t>(j, "address");
        x.base_container_id = get_stack_optional<std::string>(j, "base_container_id");
        x.bus_number = get_stack_optional<uint32_t>(j, "bus_number");
        x.bus_type_guid = get_stack_optional<std::string>(j, "bus_type_guid");
        x.capabilities = get_stack_optional<uint32_t>(j, "capabilities");
        x.characteristics = get_stack_optional<uint32_t>(j, "characteristics");
        x.class_guid = get_stack_optional<std::string>(j, "class_guid");
        x.class_name = get_stack_optional<std::string>(j, "class_name");
        x.compatible_ids = get_stack_optional<std::vector<std::string>>(j, "compatible_ids");
        x.config_flags = get_stack_optional<uint32_t>(j, "config_flags");
        x.dev_type = get_stack_optional<uint32_t>(j, "dev_type");
        x.device_desc = get_stack_optional<std::string>(j, "device_desc");
        x.device_path_mixed_case = j.at("device_path_mixed_case").get<std::string>();
        x.driver = get_stack_optional<std::string>(j, "driver");
        x.enumerator_name = get_stack_optional<std::string>(j, "enumerator_name");
        x.friendly_name = get_stack_optional<std::string>(j, "friendly_name");
        x.hardware_id = get_stack_optional<std::vector<std::string>>(j, "hardware_id");
        x.instance_id = get_stack_optional<std::string>(j, "instance_id");
        x.legacy_bus_type = get_stack_optional<uint32_t>(j, "legacy_bus_type");
        x.location_information = get_stack_optional<std::string>(j, "location_information");
        x.location_paths = get_stack_optional<std::vector<std::string>>(j, "location_paths");
        x.mfg = get_stack_optional<std::string>(j, "mfg");
        x.physical_device_object_name = get_stack_optional<std::string>(j, "physical_device_object_name");
        x.service = get_stack_optional<std::string>(j, "service");
        x.ui_number = get_stack_optional<uint32_t>(j, "ui_number");
        x.ui_number_desc_format = get_stack_optional<std::string>(j, "ui_number_desc_format");
    }

    inline void to_json(json & j, const WinSetupApiDevice & x) {
        j = json::object();
        if (x.address) {
            j["address"] = x.address;
        }
        if (x.base_container_id) {
            j["base_container_id"] = x.base_container_id;
        }
        if (x.bus_number) {
            j["bus_number"] = x.bus_number;
        }
        if (x.bus_type_guid) {
            j["bus_type_guid"] = x.bus_type_guid;
        }
        if (x.capabilities) {
            j["capabilities"] = x.capabilities;
        }
        if (x.characteristics) {
            j["characteristics"] = x.characteristics;
        }
        if (x.class_guid) {
            j["class_guid"] = x.class_guid;
        }
        if (x.class_name) {
            j["class_name"] = x.class_name;
        }
        if (x.compatible_ids) {
            j["compatible_ids"] = x.compatible_ids;
        }
        if (x.config_flags) {
            j["config_flags"] = x.config_flags;
        }
        if (x.dev_type) {
            j["dev_type"] = x.dev_type;
        }
        if (x.device_desc) {
            j["device_desc"] = x.device_desc;
        }
        j["device_path_mixed_case"] = x.device_path_mixed_case;
        if (x.driver) {
            j["driver"] = x.driver;
        }
        if (x.enumerator_name) {
            j["enumerator_name"] = x.enumerator_name;
        }
        if (x.friendly_name) {
            j["friendly_name"] = x.friendly_name;
        }
        if (x.hardware_id) {
            j["hardware_id"] = x.hardware_id;
        }
        if (x.instance_id) {
            j["instance_id"] = x.instance_id;
        }
        if (x.legacy_bus_type) {
            j["legacy_bus_type"] = x.legacy_bus_type;
        }
        if (x.location_information) {
            j["location_information"] = x.location_information;
        }
        if (x.location_paths) {
            j["location_paths"] = x.location_paths;
        }
        if (x.mfg) {
            j["mfg"] = x.mfg;
        }
        if (x.physical_device_object_name) {
            j["physical_device_object_name"] = x.physical_device_object_name;
        }
        if (x.service) {
            j["service"] = x.service;
        }
        if (x.ui_number) {
            j["ui_number"] = x.ui_number;
        }
        if (x.ui_number_desc_format) {
            j["ui_number_desc_format"] = x.ui_number_desc_format;
        }
    }

    inline void from_json(const json & j, WinSetupApiDeviceCatalog& x) {
        x.adapters = j.at("adapters").get<std::vector<WinSetupApiDevice>>();
        x.monitors = j.at("monitors").get<std::vector<WinSetupApiDevice>>();
    }

    inline void to_json(json & j, const WinSetupApiDeviceCatalog & x) {
        j = json::object();
        j["adapters"] = x.adapters;
        j["monitors"] = x.monitors;
    }

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
        x.adapter_friendly_name = get_stack_optional<std::string>(j, "adapter_friendly_name");
        x.adapter_hardware_id = get_stack_optional<std::string>(j, "adapter_hardware_id");
        x.adapter_instance_id = get_stack_optional<std::string>(j, "adapter_instance_id");
        x.adapter_registry_key = get_stack_optional<std::string>(j, "adapter_registry_key");
        x.advanced_color_info = get_stack_optional<WinAdvancedColorInfo>(j, "advanced_color_info");
        x.bounds = j.at("bounds").get<WinScreenRectangle>();
        x.dpi_scaling_percent = get_stack_optional<uint32_t>(j, "dpi_scaling_percent");
        x.edid_info = get_stack_optional<WinEdidInfo>(j, "edid_info");
        x.edid_key = get_stack_optional<std::string>(j, "edid_key");
        x.friendly_name = get_stack_optional<std::string>(j, "friendly_name");
        x.is_attached_to_desktop = get_stack_optional<bool>(j, "is_attached_to_desktop");
        x.is_primary = j.at("is_primary").get<bool>();
        x.monitor_device_path = get_stack_optional<std::string>(j, "monitor_device_path");
        x.monitor_driver_key = get_stack_optional<std::string>(j, "monitor_driver_key");
        x.monitor_instance_id = get_stack_optional<std::string>(j, "monitor_instance_id");
        x.monitor_path_key = get_stack_optional<std::string>(j, "monitor_path_key");
        x.monitor_registry_key = get_stack_optional<std::string>(j, "monitor_registry_key");
        x.monitor_serial_string = get_stack_optional<std::string>(j, "monitor_serial_string");
        x.monitor_string = get_stack_optional<std::string>(j, "monitor_string");
        x.physical_connector_type = get_stack_optional<WinDisplayConnectorType>(j, "physical_connector_type");
        x.primary_port_key = get_stack_optional<std::string>(j, "primary_port_key");
        x.refresh_rate_denominator = get_stack_optional<uint32_t>(j, "refresh_rate_denominator");
        x.refresh_rate_hz = get_stack_optional<double>(j, "refresh_rate_hz");
        x.refresh_rate_numerator = get_stack_optional<uint32_t>(j, "refresh_rate_numerator");
        x.rotation_deg = get_stack_optional<WinDisplayRotationDegrees>(j, "rotation_deg");
        x.scan_line_ordering = get_stack_optional<WinScanLineOrder>(j, "scan_line_ordering");
        x.setup_api_devices = j.at("setup_api_devices").get<std::vector<WinSetupApiDeviceCatalog>>();
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
        if (x.adapter_friendly_name) {
            j["adapter_friendly_name"] = x.adapter_friendly_name;
        }
        if (x.adapter_hardware_id) {
            j["adapter_hardware_id"] = x.adapter_hardware_id;
        }
        if (x.adapter_instance_id) {
            j["adapter_instance_id"] = x.adapter_instance_id;
        }
        if (x.adapter_registry_key) {
            j["adapter_registry_key"] = x.adapter_registry_key;
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
        if (x.monitor_driver_key) {
            j["monitor_driver_key"] = x.monitor_driver_key;
        }
        if (x.monitor_instance_id) {
            j["monitor_instance_id"] = x.monitor_instance_id;
        }
        if (x.monitor_path_key) {
            j["monitor_path_key"] = x.monitor_path_key;
        }
        if (x.monitor_registry_key) {
            j["monitor_registry_key"] = x.monitor_registry_key;
        }
        if (x.monitor_serial_string) {
            j["monitor_serial_string"] = x.monitor_serial_string;
        }
        if (x.monitor_string) {
            j["monitor_string"] = x.monitor_string;
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
        j["setup_api_devices"] = x.setup_api_devices;
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
        x.all_setup_api_devices = j.at("all_setup_api_devices").get<WinSetupApiDeviceCatalog>();
        x.displays = j.at("displays").get<std::vector<WinDisplay>>();
        x.has_interactive_desktop = j.at("has_interactive_desktop").get<bool>();
        x.is_remote_desktop = j.at("is_remote_desktop").get<bool>();
        x.is_virtual_machine = j.at("is_virtual_machine").get<bool>();
    }

    inline void to_json(json & j, const WinDisplayProberJson & x) {
        j = json::object();
        j["all_setup_api_devices"] = x.all_setup_api_devices;
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
