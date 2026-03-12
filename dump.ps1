# Windows PowerShell v2.0 or newer
#Requires -Version 2.0

# Parent directory of this script (dump.ps1).
$DumpScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

. (Join-Path -Path $DumpScriptRoot -ChildPath 'scripts/power_utils.ps1')

$HasGetCimInstance = Test-HasGetCimInstance

# Resolve file paths relative to the parent directory of this script (dump.ps1).
function Resolve-ScriptPath {
  param(
    [string]$Path
  )

  if ([System.IO.Path]::IsPathRooted($Path)) {
    return $Path
  }

  return (Join-Path -Path $DumpScriptRoot -ChildPath $Path)
}

$dp4w = (Resolve-ScriptPath -Path bin/DisplayProber-x86.exe)

function Get-DumpDir {
  $OsFullName = Get-OsNameAndVersion

  $dp4w_ver = (& $dp4w --version).Trim()

  # "2026-03-12T10:08:45-0500" -> "20260312T100845"
  $dp4w_time = (& $dp4w --build).Trim() -replace '[+-]\d{4}$', '' -replace '[-:]', ''

  $dp4w_count = (& $dp4w --count).Trim()

  # "VM" | "RDP" | "HEADLESS" | "PC"
  $dp4w_env = (& $dp4w --env).Trim()

  $nt_cv = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion'

  # 5.1
  $nt_ver = $nt_cv.CurrentVersion

  return "$DumpScriptRoot/dumps/$dp4w_ver/$dp4w_time/$nt_ver/$OsFullName/${dp4w_env}_${dp4w_count}"
}

$DumpDir = Get-DumpDir

& $dp4w |
  Write-Utf8File -Path $DumpDir/DisplayProber-x86-dev.json

# No output over RDP or SSH. Requires a physical in-person session.
# Fails in a Windows 7 SP1 guest VM.
& (Resolve-ScriptPath -Path tools/bin/win32/dumpedid/DumpEDID.exe) -a |
  Write-Utf8File -Path $DumpDir/DumpEDID.txt

# No output over RDP or SSH. Requires a physical in-person session.
# Fails in a Windows 7 SP1 guest VM.
$monitorInfoViewOutput = Ensure-ParentDirectory -Path $DumpDir/MonitorInfoView.xml
& (Resolve-ScriptPath -Path tools/bin/win32/monitorinfoview/MonitorInfoView.exe) /HideInactiveMonitors 1 /sxml $monitorInfoViewOutput

# No output over RDP or SSH. Requires a physical in-person session.
# Supports Windows 7 SP1 guest VM.
$multiMonitorToolOutput = Ensure-ParentDirectory -Path $DumpDir/MultiMonitorTool.xml
& (Resolve-ScriptPath -Path tools/bin/win32/multimonitortool/MultiMonitorTool.exe) /HideInactiveMonitors 1 /sxml $multiMonitorToolOutput

# No output over RDP or SSH. Requires a physical in-person session.
# Supports Windows 7 SP1 guest VM.
$controlMyMonitorOutput = Ensure-ParentDirectory -Path $DumpDir/ControlMyMonitor.txt
& (Resolve-ScriptPath -Path tools/bin/win32/controlmymonitor/ControlMyMonitor.exe) /smonitors $controlMyMonitorOutput

Convert-FileToUtf8 -Path $monitorInfoViewOutput
Convert-FileToUtf8 -Path $multiMonitorToolOutput
Convert-FileToUtf8 -Path $controlMyMonitorOutput

# Windows PowerShell v3.0 and above (Windows 8+)
if ($HasGetCimInstance) {
  Get-CimInstance Win32_PnPEntity |
    Where-Object { @('monitor') -contains $_.Service } |
    ConvertTo-Json -Depth 4 |
    Write-Utf8File -Path $DumpDir/Win32_PnPEntity_Service.json

  # Over SSH, this returns a default "dummy" display:
  # `"DeviceName": "WinDisc"` at 1024x768.
  Add-Type -AssemblyName System.Windows.Forms
  [System.Windows.Forms.Screen]::AllScreens |
    ConvertTo-Json -Depth 4 |
    Write-Utf8File -Path $DumpDir/WinFormsScreens.json

  # Does not require an interactive graphical session.
  # Returns physical displays even over SSH.
  Get-CimInstance Win32_VideoController |
    ConvertTo-Json -Depth 4 |
    Write-Utf8File -Path $DumpDir/Win32_VideoController.json

  # Does not require an interactive graphical session.
  # Returns physical displays even over SSH.
  Get-CimInstance Win32_PnPEntity |
    Where-Object { @('Monitor', 'Display') -contains $_.PNPClass } |
    ConvertTo-Json -Depth 4 |
    Write-Utf8File -Path $DumpDir/Win32_PnPEntity_Class.json

  # Does not require an interactive graphical session.
  # Returns physical displays even over SSH.
  Get-CimInstance -Namespace root\wmi WmiMonitorBasicDisplayParams |
    ConvertTo-Json -Depth 4 |
    Write-Utf8File -Path $DumpDir/WmiMonitorBasicDisplayParams.json

  # Does not require an interactive graphical session.
  # Returns physical displays even over SSH.
  Get-CimInstance -Namespace root\wmi WmiMonitorConnectionParams |
    ConvertTo-Json -Depth 4 |
    Write-Utf8File -Path $DumpDir/WmiMonitorConnectionParams.json

  # Does not require an interactive graphical session.
  # Returns physical displays even over SSH.
  Get-CimInstance -Namespace root\wmi WmiMonitorDescriptorMethods |
    ConvertTo-Json -Depth 4 |
    Write-Utf8File -Path $DumpDir/WmiMonitorDescriptorMethods.json

  # Does not require an interactive graphical session.
  # Returns physical displays even over SSH.
  Get-CimInstance -Namespace root\wmi WmiMonitorID |
    ConvertTo-Json -Depth 4 |
    Write-Utf8File -Path $DumpDir/WmiMonitorID.json
}
# Windows PowerShell v2.0 (Windows 7)
else {
  Get-WmiObject Win32_PnPEntity |
    Where-Object { @('monitor') -contains $_.Service } |
    Write-Utf8File -Path $DumpDir/Win32_PnPEntity_Service.txt
}
