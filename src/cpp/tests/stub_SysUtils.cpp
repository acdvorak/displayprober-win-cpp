#include "SysUtils.h"

namespace sys {

bool IsRdpSession() { return false; }

bool IsVirtualMachine() { return false; }

bool HasInteractiveDesktop() { return true; }

bool is_win_8dot1_or_newer() { return true; }

bool is_win_10_v16070_or_newer() { return true; }

bool is_win_11_v24H2_or_newer() { return false; }

}  // namespace sys
