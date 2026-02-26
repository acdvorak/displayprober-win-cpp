// This header needs to be imported first.
#include <Windows.h>

#include <iostream>
#include <string>
#include <string_view>

#include "DisplayProberLib.h"
#include "StringUtils.h"

#ifndef DP4W_VERSION_TAG
#define DP4W_VERSION_TAG "v0.0.0"
#endif

#ifndef DP4W_GIT_COMMIT
#define DP4W_GIT_COMMIT "unknown"
#endif

#ifndef DP4W_BUILD_TIMESTAMP
#define DP4W_BUILD_TIMESTAMP "unknown"
#endif

static std::wstring Utf8ToWide(std::string_view value) {
  if (value.empty()) {
    return {};
  }

  const int required = MultiByteToWideChar(
      CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
  if (required <= 0) {
    return {};
  }

  std::wstring wide(static_cast<size_t>(required), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                      wide.data(), required);
  return wide;
}

static bool IsHelpArg(std::wstring_view arg) {
  if (arg == L"-h" || arg == L"--help") {
    return true;
  }
  return EqualsIgnoreCase(arg, L"/h") || EqualsIgnoreCase(arg, L"/help");
}

static void PrintUsage() {
  std::wcout
      << L"DisplayProber: CLI enumerate connected/enabled displays.\n"
      << L"\n"
      << L"Usage: DisplayProber [-h|--help|/h|/Help]\n"
      << L"\n"
      << L"Options:\n"
      << L"  -h | --help    Show this help message\n"
      << L"  /h | /Help\n"
      << L"\n"
      << L"Windows version support:\n"
      << L"  - Windows 7+:       Display names, resolution, refresh rate, "
         L"connector type\n"
      << L"  - Windows 8.1+:     Per-monitor DPI scaling\n"
      << L"  - Windows 10 1607+: HDR/advanced color support + per-thread DPI "
         L"awareness\n"
      << L"  - Windows 11 24H2+: wide color support state + active color mode\n"
      << L"\n"
      << L"Detected features:\n"
      << L"  - Display/monitor names, resolution, refresh rate\n"
      << L"  - Connector type (VGA/DVI/HDMI/DisplayPort/internal)\n"
      << L"  - DPI scaling percentage\n"
      << L"  - Color encoding/bit depth, HDR, advanced color, DXGI color "
         L"space\n"
      << L"\n"
      << L"Build metadata:\n"
      << L"  Version: " << Utf8ToWide(DP4W_VERSION_TAG) << L"\n"
      << L"  Commit:  " << Utf8ToWide(DP4W_GIT_COMMIT) << L"\n"
      << L"  Built:   " << Utf8ToWide(DP4W_BUILD_TIMESTAMP) << L"\n"
      << L"\n"
      << L"https://github.com/acdvorak/displayprober-win-cpp" << "\n"
      << L"";
}

int wmain(int argc, wchar_t* argv[]) {
  for (int i = 1; i < argc; ++i) {
    std::wstring_view arg = argv[i];
    if (IsHelpArg(arg)) {
      PrintUsage();
      return 0;
    } else {
      std::wcerr << L"Unknown option: " << arg << L"\n\n";
      PrintUsage();
      return 1;
    }
  }

  std::cout << GetDisplayProberJson() << std::endl;

  return 0;
}
