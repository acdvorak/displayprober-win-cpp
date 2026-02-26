#pragma once

namespace sys {

// Indicates whether the process is running inside a Remote Desktop (RDP)
// session.
//
// Does NOT include other remote-control protocols like VNC, RustDesk,
// TeamViewer, Chrome Remote Desktop, etc.
//
// RDP is "special" because it creates a completely separate *virtual* session
// that is not tied to a physical display (monitor/TV), which makes it suitable
// for managing servers.
//
// Most other remoting tools simply mirror whatever's already shown on
// physical screen(s).
bool IsRdpSession();

// Indicates whether the process *probably* running inside a virtual machine
// such as VMware Workstation, Parallels Desktop, VirtualBox, etc.
bool IsVirtualMachine();

// Indicates whether the process has an interactive graphical UI.
//
// This will only be `false` for SSH terminals, remote consoles, and other types
// of headless sessions.
bool HasInteractiveDesktop();

// At least: Windows 8.1 (workstation) or Windows Server 2012 R2.
bool is_win_8dot1_or_newer();

// At least: Windows 10 version 1607.
bool is_win_10_v16070_or_newer();

// At least: Windows 11 version 24H2.
bool is_win_11_v24H2_or_newer();

}  // namespace sys
