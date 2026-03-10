#pragma once

// This header needs to be imported first.
#include <Windows.h>

// Keep other .h headers separate from Windows.h to prevent auto-sorting.
#include <SetupAPI.h>

class ScopedDevInfoSet {
 public:
  explicit ScopedDevInfoSet(HDEVINFO handle) : handle_(handle) {}

  ~ScopedDevInfoSet() {
    if (handle_ != INVALID_HANDLE_VALUE) {
      SetupDiDestroyDeviceInfoList(handle_);
    }
  }

  HDEVINFO get() const { return handle_; }

 private:
  HDEVINFO handle_ = INVALID_HANDLE_VALUE;
};

class ScopedRegKey {
 public:
  explicit ScopedRegKey(HKEY key) : key_(key) {}

  ~ScopedRegKey() {
    if (key_ != nullptr && key_ != INVALID_HANDLE_VALUE) {
      RegCloseKey(key_);
    }
  }

  HKEY get() const { return key_; }

 private:
  HKEY key_ = nullptr;
};
