// This header needs to be imported first.
#include <Windows.h>

#include <iostream>
#include <string>
#include <string_view>

#include "DisplayProberLib.h"
#include "GdiMonitorEnum.h"
#include "StringUtils.h"
#include "SysUtils.h"

#ifndef DP4W_VERSION_TAG
#define DP4W_VERSION_TAG "v0.0.0"
#endif

#ifndef DP4W_GIT_COMMIT
#define DP4W_GIT_COMMIT "unknown"
#endif

#ifndef DP4W_BUILD_TIMESTAMP
#define DP4W_BUILD_TIMESTAMP "unknown"
#endif

static bool IsHelpArg(std::wstring_view arg) {
  if (arg == L"/?" || arg == L"-h" || arg == L"--help") {
    return true;
  }
  return EqualsIgnoreCase(arg, L"/h") || EqualsIgnoreCase(arg, L"/help");
}

static std::string GetEnv() {
  if (sys::IsVirtualMachine()) {
    return "VM";
  }
  if (sys::IsRdpSession()) {
    return "RDP";
  }
  if (!sys::HasInteractiveDesktop()) {
    return "HEADLESS";
  }
  return "PC";
}

static void PrintUsage() {
  std::cout << "DisplayProber: CLI enumerate connected/enabled displays.\n"
            << "\n"
            << "Usage: DisplayProber\n"
            << "       DisplayProber [--version] [--commit] [--build]\n"
            << "       DisplayProber [-h|--help|/h|/Help|/?]\n"
            << "\n"
            << "Options:\n"
            << "  -h | --help    Show this help message\n"
            << "  /h | /Help | /?\n"
            << "  --version      Print version tag\n"
            << "  --commit       Print git commit hash\n"
            << "  --build        Print build timestamp\n"
            << "  --env          Print \"VM\", \"RDP\", \"HEADLESS\", \"PC\"\n"
            << "  --count        Print the number of active monitors\n"
            << "\n"
            << "Build metadata:\n"
            << "  Version: " << DP4W_VERSION_TAG << "\n"
            << "  Commit:  " << DP4W_GIT_COMMIT << "\n"
            << "  Built:   " << DP4W_BUILD_TIMESTAMP << "\n"
            << "\n"
            << "https://github.com/acdvorak/displayprober-win-cpp" << "\n"
            << "";
}

int wmain(int argc, wchar_t* argv[]) {
  // Set console output code page to UTF-8
  SetConsoleOutputCP(CP_UTF8);

  for (int i = 1; i < argc; ++i) {
    std::wstring_view arg = argv[i];
    if (IsHelpArg(arg)) {
      PrintUsage();
      return 0;
    } else if (arg == L"--version") {
      std::cout << DP4W_VERSION_TAG << "\n";
      return 0;
    } else if (arg == L"--commit") {
      std::cout << DP4W_GIT_COMMIT << "\n";
      return 0;
    } else if (arg == L"--build") {
      std::cout << DP4W_BUILD_TIMESTAMP << "\n";
      return 0;
    } else if (arg == L"--env") {
      std::cout << GetEnv() << "\n";
      return 0;
    } else if (arg == L"--count") {
      std::cout << gdi::GetGdiMonitorInfos().size() << "\n";
      return 0;
    } else {
      std::cerr << "Unknown option: " << WideToUtf8(arg) << "\n\n";
      PrintUsage();
      return 1;
    }
  }

  std::cout << GetDisplayProberJson() << std::endl;

  return 0;
}
