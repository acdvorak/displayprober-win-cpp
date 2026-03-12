#pragma once

#include <memory>
#include <type_traits>

// This header needs to be imported first.
#include <Windows.h>

// Keep other .h headers separate from Windows.h to prevent auto-sorting.
#include <SetupAPI.h>

struct DevInfoSetDeleter {
  using pointer = HDEVINFO;
  void operator()(pointer handle) const {
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
      SetupDiDestroyDeviceInfoList(handle);
    }
  }
};

using UniqueDevInfoSet = std::unique_ptr<void, DevInfoSetDeleter>;

struct RegKeyDeleter {
  using pointer = HKEY;
  void operator()(pointer key) const {
    if (key != nullptr && key != INVALID_HANDLE_VALUE) {
      RegCloseKey(key);
    }
  }
};

using UniqueRegKey =
    std::unique_ptr<std::remove_pointer_t<HKEY>, RegKeyDeleter>;
