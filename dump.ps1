# Windows PowerShell v2.0 or newer
#Requires -Version 2.0

# Parent directory of this script (dump.ps1).
$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

# Resolve file paths relative to the parent directory of this script (dump.ps1).
function Resolve-ScriptPath {
  param(
    [string]$Path
  )

  if ([System.IO.Path]::IsPathRooted($Path)) {
    return $Path
  }

  return (Join-Path -Path $ScriptRoot -ChildPath $Path)
}

# mkdir -p $(dirname $Path)
function Ensure-ParentDirectory {
  param(
    [string]$Path
  )

  $resolvedPath = Resolve-ScriptPath -Path $Path
  $parentPath = Split-Path -Parent $resolvedPath

  if ($parentPath -and -not (Test-Path -LiteralPath $parentPath)) {
    New-Item -ItemType Directory -Path $parentPath -Force | Out-Null
  }

  return $resolvedPath
}

# Convert stdin from the Windows default encoding (UTF-16 LE) to UTF-8.
function Write-Utf8File {
  param(
    [string]$Path,
    [Parameter(ValueFromPipeline = $true)]
    $InputObject
  )

  begin {
    $resolvedPath = Ensure-ParentDirectory -Path $Path

    if (Test-Path -LiteralPath $resolvedPath) {
      Remove-Item -LiteralPath $resolvedPath -Force
    }
  }

  process {
    $InputObject | Out-File -FilePath $resolvedPath -Encoding UTF8 -Append
  }
}

# Re-encode a text file from the Windows default (UTF-16 LE) to UTF-8.
function Convert-FileToUtf8 {
  param(
    [string]$Path
  )

  $resolvedPath = Ensure-ParentDirectory -Path $Path

  if (-not (Test-Path -LiteralPath $resolvedPath)) {
    return
  }

  $text = [System.IO.File]::ReadAllText($resolvedPath)
  [System.IO.File]::WriteAllText($resolvedPath, $text, [System.Text.Encoding]::UTF8)
}

$OsObject = Get-WmiObject Win32_OperatingSystem
$OsBaseName = $OsObject.Caption.Trim() -replace '^Microsoft ', ''
$ServicePack = if ($OsObject.ServicePackMajorVersion -gt 0) {
  ' SP{0}' -f $OsObject.ServicePackMajorVersion
}
else {
  ''
}
$OsFullName = '{0}{1} (build {2})' -f $OsBaseName, $ServicePack, $OsObject.BuildNumber
$DumpDir = ".\dumps\$OsFullName"

# No output over RDP or SSH. Requires a physical in-person session.
& (Resolve-ScriptPath -Path .\tools\bin\win32\dumpedid\DumpEDID.exe) -a |
  Write-Utf8File -Path $DumpDir\DumpEDID.txt

# No output over RDP or SSH. Requires a physical in-person session.
$monitorInfoViewOutput = Ensure-ParentDirectory -Path $DumpDir\MonitorInfoView.xml
& (Resolve-ScriptPath -Path .\tools\bin\win32\monitorinfoview\MonitorInfoView.exe) /HideInactiveMonitors 1 /sxml $monitorInfoViewOutput

# No output over RDP or SSH. Requires a physical in-person session.
$multiMonitorToolOutput = Ensure-ParentDirectory -Path $DumpDir\MultiMonitorTool.xml
& (Resolve-ScriptPath -Path .\tools\bin\win32\multimonitortool\MultiMonitorTool.exe) /HideInactiveMonitors 1 /sxml $multiMonitorToolOutput

# No output over RDP or SSH. Requires a physical in-person session.
$controlMyMonitorOutput = Ensure-ParentDirectory -Path $DumpDir\ControlMyMonitor.txt
& (Resolve-ScriptPath -Path .\tools\bin\win32\controlmymonitor\ControlMyMonitor.exe) /smonitors $controlMyMonitorOutput

Convert-FileToUtf8 -Path $DumpDir\MonitorInfoView.xml
Convert-FileToUtf8 -Path $DumpDir\MultiMonitorTool.xml
Convert-FileToUtf8 -Path $DumpDir\ControlMyMonitor.txt

# Over SSH, this returns a default "dummy" display:
# `"DeviceName": "WinDisc"` at 1024x768.
Add-Type -AssemblyName System.Windows.Forms
[System.Windows.Forms.Screen]::AllScreens |
  ConvertTo-Json |
  Write-Utf8File -Path $DumpDir\WinFormsScreens.json

# Does not require an interactive graphical session.
# Returns physical displays even over SSH.
Get-CimInstance Win32_VideoController |
  ConvertTo-Json |
  Write-Utf8File -Path $DumpDir\Win32_VideoController.json

# Does not require an interactive graphical session.
# Returns physical displays even over SSH.
Get-CimInstance Win32_PnPEntity |
  Where-Object { $_.PNPClass -in @('Monitor', 'Display') } |
  ConvertTo-Json |
  Write-Utf8File -Path $DumpDir\Win32_PnPEntity.json

# Does not require an interactive graphical session.
# Returns physical displays even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorBasicDisplayParams |
  ConvertTo-Json |
  Write-Utf8File -Path $DumpDir\WmiMonitorBasicDisplayParams.json

# Does not require an interactive graphical session.
# Returns physical displays even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorConnectionParams |
  ConvertTo-Json |
  Write-Utf8File -Path $DumpDir\WmiMonitorConnectionParams.json

# Does not require an interactive graphical session.
# Returns physical displays even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorDescriptorMethods |
  ConvertTo-Json |
  Write-Utf8File -Path $DumpDir\WmiMonitorDescriptorMethods.json

# Does not require an interactive graphical session.
# Returns physical displays even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorID |
  ConvertTo-Json |
  Write-Utf8File -Path $DumpDir\WmiMonitorID.json
