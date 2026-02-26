#include "SysUtils.h"

// This header needs to be imported first.
#include <Windows.h>

// Keep other .h headers separate from Windows.h to prevent auto-sorting.
#include <WtsApi32.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "StringUtils.h"

namespace {

const OSVERSIONINFOEXW GetWindowsVersionUncached() {
  OSVERSIONINFOEXW osInfo = {sizeof(osInfo)};
  typedef NTSTATUS(WINAPI * PFN_RtlGetVersion)(PRTL_OSVERSIONINFOEXW);
  static auto pRtlGetVersion = reinterpret_cast<PFN_RtlGetVersion>(
      GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
  if (pRtlGetVersion) {
    pRtlGetVersion(&osInfo);
  }

  return osInfo;
}

const OSVERSIONINFOEXW& GetWindowsVersionCached() {
  static const OSVERSIONINFOEXW osInfo = GetWindowsVersionUncached();
  return osInfo;
}

bool IsRdpSessionUncached() {
  if (GetSystemMetrics(SM_REMOTESESSION) != 0) {
    return true;
  }

  USHORT* protocolType = nullptr;
  DWORD bytesReturned = 0;

  if (WTSQuerySessionInformationW(
          WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSClientProtocolType,
          reinterpret_cast<LPWSTR*>(&protocolType), &bytesReturned) == FALSE) {
    return false;
  }

  bool isRdp = false;
  if (protocolType && bytesReturned >= sizeof(USHORT)) {
    isRdp = (*protocolType == 2);
  }

  WTSFreeMemory(protocolType);
  return isRdp;
}

// List of strings returned by the BIOS.
//
// System Management BIOS (SMBIOS) is a DMTF-standardized method for PC firmware
// to communicate detailed hardware information (such as manufacturer, model,
// serial numbers, BIOS version, and memory configuration) to operating systems.
const std::vector<std::string> GetSmBiosStringsUncached() {
  constexpr DWORD kRawSmbiosTableId = 'RSMB';

  const UINT size = GetSystemFirmwareTable(kRawSmbiosTableId, 0, nullptr, 0);
  if (size == 0) {
    return {};
  }

  std::vector<std::uint8_t> raw(size, 0);
  if (GetSystemFirmwareTable(kRawSmbiosTableId, 0, raw.data(), size) != size) {
    return {};
  }

  struct RawSmbiosData {
    std::uint8_t used20CallingMethod;
    std::uint8_t smbiosMajorVersion;
    std::uint8_t smbiosMinorVersion;
    std::uint8_t dmiRevision;
    std::uint32_t length;
    std::uint8_t tableData[1];
  };

  if (raw.size() < sizeof(RawSmbiosData)) {
    return {};
  }

  const auto* smbios = reinterpret_cast<const RawSmbiosData*>(raw.data());
  const auto* table = smbios->tableData;
  const auto* cursor = table;
  const auto* end = table + smbios->length;

  if (end < table || end > raw.data() + raw.size()) {
    return {};
  }

  std::vector<std::string> strings;

  while (cursor + 4 <= end) {
    const std::uint8_t type = cursor[0];
    const std::uint8_t length = cursor[1];

    if (length < 4 || cursor + length > end) {
      break;
    }

    auto* strCursor = cursor + length;
    while (strCursor < end && *strCursor != 0) {
      const char* text = reinterpret_cast<const char*>(strCursor);
      size_t textLen = 0;
      while (strCursor + textLen < end && strCursor[textLen] != 0) {
        ++textLen;
      }

      if (textLen > 0) {
        strings.emplace_back(text, textLen);
      }

      strCursor += textLen;
      if (strCursor < end && *strCursor == 0) {
        ++strCursor;
      }
    }

    if (strCursor + 1 >= end) {
      break;
    }

    cursor = strCursor + 2;
    if (type == 127) {
      break;
    }
  }

  return strings;
}

// List of strings returned by the BIOS.
//
// System Management BIOS (SMBIOS) is a DMTF-standardized method for PC firmware
// to communicate detailed hardware information (such as manufacturer, model,
// serial numbers, BIOS version, and memory configuration) to operating systems.
const std::vector<std::string>& GetSmBiosStringsCached() {
  static const std::vector<std::string> cached = GetSmBiosStringsUncached();
  return cached;
}

// clang-format off
static std::initializer_list<std::string_view> hyper_visor_tokens = {
  "ACRN",
  "APPLE",
  "APPLEHV",
  "BHYVE",
  "KVM",
  "PARALLELS",
  "PRL HYPERV",
  "PRLS",
  "QEMU",
  "TCG",
  "VBOX",
  "VMWARE",
  "XEN",
};
// clang-format on

// clang-format off
static std::initializer_list<std::string_view> sm_bios_tokens = {
  "ACRN",
  "APPLE VIRTUALIZATION",
  "BHYVE",
  "BOCHS",
  "HVM DOMU",
  "HYPER-V",
  "INNOTEK GMBH",
  "KVM",
  "LIBVIRT",
  "OPENSTACK",
  "ORACLE VM VIRTUALBOX",
  "OVIRT",
  "PARALLELS SOFTWARE",
  "PARALLELS",
  "PROXMOX",
  "Q35",
  "QEMU",
  "RHEV",
  "VBOX",
  "VIRTUAL BOX",
  "VIRTUAL MACHINE",
  "VIRTUAL PC",
  "VIRTUALBOX",
  "VMWARE FUSION",
  "VMWARE, INC.",
  "VMWARE",
  "XEN",
};
// clang-format on

bool IsVirtualMachineUncached() {
  std::array<int, 4> cpuInfo = {};

  __cpuid(cpuInfo.data(), 1);

  const bool isHyperVisorPresent = (cpuInfo[2] & (1 << 31)) != 0;

  if (isHyperVisorPresent) {
    // DO NOT RETURN TRUE HERE!
    // This creates false positives on machines that support HyperVisor hosting.
    // We only want to detect HyperVisor *clients*.
    // return true;
  }

  __cpuid(cpuInfo.data(), 0x40000000);
  std::string hyperVisorVendor(12, '\0');
  std::memcpy(hyperVisorVendor.data(), &cpuInfo[1], 4);
  std::memcpy(hyperVisorVendor.data() + 4, &cpuInfo[2], 4);
  std::memcpy(hyperVisorVendor.data() + 8, &cpuInfo[3], 4);

  if (ContainsAnyToken(hyperVisorVendor, hyper_visor_tokens)) {
    return true;
  }

  std::string concatenatedSmBiosStrings;
  for (const auto& entry : GetSmBiosStringsCached()) {
    if (!concatenatedSmBiosStrings.empty()) {
      concatenatedSmBiosStrings.push_back(' ');
    }
    concatenatedSmBiosStrings.append(entry);
  }

  return ContainsAnyToken(concatenatedSmBiosStrings, sm_bios_tokens);
}

bool HasInteractiveDesktopUncached() {
  HDESK desktop = OpenInputDesktop(0, FALSE, GENERIC_READ);
  if (!desktop) {
    return false;
  }
  CloseDesktop(desktop);
  return true;
}

}  // namespace

namespace sys {

bool IsRdpSession() {
  static const bool cached = IsRdpSessionUncached();
  return cached;
}

bool IsVirtualMachine() {
  static const bool cached = IsVirtualMachineUncached();
  return cached;
}

bool HasInteractiveDesktop() {
  static const bool cached = HasInteractiveDesktopUncached();
  return cached;
}

// Windows 8.1 (build 9600) or newer.
//
// Released October 17, 2013.
bool is_win_8dot1_or_newer() {
  static const bool bis_win_8dot1_or_newer = [] {
    const auto& osInfo = GetWindowsVersionCached();
    return osInfo.dwMajorVersion > 6 ||
           (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 3);
  }();
  return bis_win_8dot1_or_newer;
}

// Windows 10 version 1607 (build 14393) or newer.
//
// Released August 2, 2016.
bool is_win_10_v16070_or_newer() {
  static const bool bis_win_10_v16070_or_newer = [] {
    const auto& osInfo = GetWindowsVersionCached();
    return osInfo.dwMajorVersion > 10 ||
           (osInfo.dwMajorVersion == 10 && osInfo.dwBuildNumber >= 14393);
  }();
  return bis_win_10_v16070_or_newer;
}

// Windows 11 version 24H2 (build 26100) or newer.
//
// Released October 1, 2024.
bool is_win_11_v24H2_or_newer() {
  static const bool bis_win_11_v24H2_or_newer = [] {
    const auto& osInfo = GetWindowsVersionCached();
    return osInfo.dwMajorVersion > 10 ||
           (osInfo.dwMajorVersion == 10 && osInfo.dwBuildNumber >= 26100);
  }();
  return bis_win_11_v24H2_or_newer;
}

}  // namespace sys
