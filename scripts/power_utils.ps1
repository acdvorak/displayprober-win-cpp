# Windows PowerShell v2.0 or newer
#Requires -Version 2.0

# Parent directory of this script (power_utils.ps1).
$PuScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

function Get-AbsPath {
  param(
    [string]$From,
    [string]$To
  )

  if ([System.IO.Path]::IsPathRooted($To)) {
    return $To
  }

  return [System.IO.Path]::GetFullPath([System.IO.Path]::Combine($From, $To))
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
    $items = @()

    if (Test-Path -LiteralPath $resolvedPath) {
      Remove-Item -LiteralPath $resolvedPath -Force
    }
  }

  process {
    $items += $InputObject
  }

  end {
    $content = $items | Out-String

    [System.IO.File]::WriteAllText(
      $resolvedPath,
      $content,
      (New-Object System.Text.UTF8Encoding($false)) # UTF-8 without BOM
    )

    Write-Host ('"{0}"' -f $resolvedPath)
  }
}

# Re-encode a text file from the Windows default (UTF-16 LE) to UTF-8.
function Convert-FileToUtf8 {
  param(
    [string]$Path,
    [int]$WaitTimeoutMs = 3000,
    [int]$PollIntervalMs = 50
  )

  $resolvedPath = Ensure-ParentDirectory -Path $Path

  $deadline = [DateTime]::UtcNow.AddMilliseconds($WaitTimeoutMs)
  $lastLength = -1
  $stableSamples = 0

  # Wait for file to be created and its size to stabilize.
  while ([DateTime]::UtcNow -lt $deadline) {
    if (Test-Path -LiteralPath $resolvedPath) {
      try {
        $length = (Get-Item -LiteralPath $resolvedPath -ErrorAction Stop).Length

        if ($length -eq $lastLength) {
          $stableSamples += 1
        }
        else {
          $stableSamples = 0
          $lastLength = $length
        }

        if ($stableSamples -ge 2) {
          break
        }
      }
      catch {
        # Ignore
      }
    }

    Start-Sleep -Milliseconds $PollIntervalMs
  }

  if (-not (Test-Path -LiteralPath $resolvedPath)) {
    return
  }

  # Ensure that NirSoft tools don't overwrite our conversion with late writes.
  for ($attempt = 0; $attempt -lt 3; $attempt++) {
    try {
      $text = [System.IO.File]::ReadAllText($resolvedPath)
      [System.IO.File]::WriteAllText(
        $resolvedPath,
        $text,
        (New-Object System.Text.UTF8Encoding($false)) # UTF-8 without BOM
      )

      Write-Host ('"{0}"' -f $resolvedPath)

      return
    }
    catch {
      Start-Sleep -Milliseconds $PollIntervalMs
    }
  }
}

if (-not ('DotNetRuntimeVer' -as [type])) {
  $resolvedPath = Get-AbsPath -From $PuScriptRoot -To '.\DotNetRuntimeVer.cs'
  $CS = [System.IO.File]::ReadAllText($resolvedPath)
  Add-Type -TypeDefinition $CS
}

function Get-ClassicDotNetRuntimeList {
  # `OutputType` is not compatible with PS 2.0
  # [OutputType([DotNetRuntimeVer[]])]

  # Lists .NET Framework versions from:
  # HKLM\SOFTWARE\Microsoft\NET Framework Setup\NDP
  # Works on Windows 7 SP1+ and PowerShell 2.0+
  $ndp = 'HKLM:\SOFTWARE\Microsoft\NET Framework Setup\NDP'

  return Get-ChildItem $ndp -Recurse |
    Get-ItemProperty -Name Version, Release, SP, Install -ErrorAction SilentlyContinue |
    Where-Object { $_.Install -eq 1 -and $_.PSChildName -match '^(v|Client|Full)' } |
    Select-Object `
    @{Name = 'Name'; Expression = { $_.PSChildName } },
    @{Name = 'Version'; Expression = { $_.Version } },
    @{Name = 'SP'; Expression = { $_.SP } },
    @{Name = 'Release'; Expression = { $_.Release } } |
    ForEach-Object {
      New-Object DotNetRuntimeVer @($_.Name, $_.Version, $_.SP, $_.Release)
    } |
    Sort-Object Version, Name
}

function Get-ModernDotNetRuntimeList {
  # `OutputType` is not compatible with PS 2.0
  # [OutputType([DotNetRuntimeVer[]])]

  $dotnet_cmd = Get-Command dotnet -ErrorAction SilentlyContinue
  if (-not $dotnet_cmd) {
    return @()
  }

  return @(dotnet --list-runtimes 2>$null |
      ForEach-Object {
        if ($_ -match '^\s*([^\s]+)\s+([^\s]+)\s+\[.*\]\s*$') {
          $name = $matches[1]
          $version = $matches[2]
          New-Object DotNetRuntimeVer @($name, $version)
        }
      } |
      Sort-Object Name, Version)
}

function Test-HasGetCimInstance {
  return [bool](Get-Command Get-CimInstance -ErrorAction SilentlyContinue)
}

# Example values:
#
# - Windows 7 Pro SP1 (build 7601)
# - Windows Server 2012 R2 Standard Evaluation (build 9600)
# - Windows 10 Pro 22H2 (build 19045)
# - Windows 11 Pro 25H2 (build 26200)
function Get-OsNameAndVersion {
  $has_get_cm_instance = Test-HasGetCimInstance

  $os_obj = if ($has_get_cm_instance) {
    # PowerShell v3.0+, Windows 8+
    Get-CimInstance Win32_OperatingSystem
  }
  else {
    # PowerShell v2.0, Windows 7
    Get-WmiObject Win32_OperatingSystem
  }

  # Example values (before replacements):
  #
  # - Microsoft Windows 7 Professional
  # - Microsoft Windows Server 2012 R2 Standard Evaluation
  # - Microsoft Windows 10 Pro
  # - Microsoft Windows 11 Pro
  $os_name = ($os_obj.Caption.Trim() -replace '^Microsoft ', '') -replace ' Professional', ' Pro'

  # Service Pack. Windows 7 only.
  $sp = if ($os_obj.ServicePackMajorVersion) { 'SP{0}' -f $os_obj.ServicePackMajorVersion } else { '' }

  $nt_cv = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion'

  # Modern, human-friendly "feature update version" for Windows 10+,
  # starting in the year 2020.
  #
  # Supercedes the old, obsolete Windows 10 `ReleaseId` field.
  #
  # Example values: 20H2, 21H2, 22H2, 23H2, 24H2
  $feature = if ($nt_cv.DisplayVersion) { $nt_cv.DisplayVersion } else { '' }

  # Legacy "feature update version" from early version of Windows 10.
  # Value is frozen at 2009.
  #
  # Replaced by `DisplayVersion` in Windows 10+ starting in the year 2020.
  #
  # Example values: 1507, 1607, 1809, 2004, 2009
  $release = if ($nt_cv.ReleaseId -lt 2009) { 'release {0}' -f $nt_cv.ReleaseId } else { '' }

  # All versions of windows have an incrementing build number.
  $build = '(build {0})' -f $os_obj.BuildNumber

  return (($os_name, $sp, $feature, $release, $build) |
      Where-Object { $_ } ) -join ' '
}

function Get-PsNameAndVersion {
  $major = $PSVersionTable.PSVersion.Major
  $minor = $PSVersionTable.PSVersion.Minor
  if ($PSVersionTable.Edition -eq 'Core') {
    return 'Core PowerShell v{0}.{1}' -f $major, $minor
  }
  else {
    return 'Windows PowerShell v{0}.{1}' -f $major, $minor
  }
}
