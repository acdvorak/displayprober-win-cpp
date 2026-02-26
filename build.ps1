# Windows PowerShell v5.1
#Requires -Version 5.1
#Requires -PSEdition Desktop

param(
  [switch]$Debug,
  [switch]$Release,
  [switch]$x64,
  [switch]$x86,
  [switch]$Both,
  [switch]$Clean
)

if ($Debug -and $Release) {
  throw "Parameters -Debug and -Release are mutually exclusive. Specify only one."
}

if ($Both -and ($x64 -or $x86)) {
  throw "Parameter -Both cannot be combined with -x64 or -x86."
}

if (-not ($x64 -or $x86 -or $Both)) {
  $x64 = $true
}

$Configuration = if ($Release) { "Release" } else { "Debug" }

$InformationPreference = 'Continue'

$ErrorActionPreference = "Stop"

$VsWherePath = Join-Path ${env:ProgramFiles(x86)} `
  "Microsoft Visual Studio/Installer/vswhere.exe"

if (-not (Test-Path $VsWherePath)) {
  throw "vswhere.exe was not found at: $VsWherePath"
}

$VsInstallationPath = & $VsWherePath -latest -products * `
  -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
  -property installationPath

if (-not $VsInstallationPath) {
  throw (
    "No Visual Studio Build Tools instance with VC tools was found. " +
    "Re-run deps.ps1 and ensure C++ Build Tools are installed."
  )
}

$VsInstallationPath = $VsInstallationPath.Trim()
$VsDevShellPath = Join-Path $VsInstallationPath "Common7/Tools/Launch-VsDevShell.ps1"

if (-not (Test-Path $VsDevShellPath)) {
  throw "Launch-VsDevShell.ps1 was not found at: $VsDevShellPath"
}

$CppSourceDir = Join-Path $PSScriptRoot "src/cpp"
$CMakePresetsPath = Join-Path $CppSourceDir "CMakePresets.json"

if (-not (Test-Path $CMakePresetsPath)) {
  throw "CMakePresets.json was not found at: $CMakePresetsPath"
}

$buildBothArchitectures = $Both -or ($x64 -and $x86)

$PresetBuilds = if ($buildBothArchitectures) {
  @(
    @{ ConfigurePreset = "Win32"; OutputArch = "x86" },
    @{ ConfigurePreset = "x64"; OutputArch = "x64" }
  )
}
elseif ($x86) {
  @(
    @{ ConfigurePreset = "Win32"; OutputArch = "x86" }
  )
}
else {
  @(
    @{ ConfigurePreset = "x64"; OutputArch = "x64" }
  )
}

$BinDir = Join-Path $PSScriptRoot "bin"

if (Test-Path -LiteralPath $BinDir) {
  Get-ChildItem -LiteralPath $BinDir -Force | Remove-Item -Recurse -Force
}

New-Item -ItemType Directory -Path $BinDir -Force | Out-Null

foreach ($PresetBuild in $PresetBuilds) {
  $BuildPreset = "$($PresetBuild.ConfigurePreset)-$Configuration"
  $BuildDir = Join-Path $PSScriptRoot "out/$($PresetBuild.ConfigurePreset)"
  $CMakeCachePath = Join-Path $BuildDir "CMakeCache.txt"
  $CMakeFilesDir = Join-Path $BuildDir "CMakeFiles"

  if ($Clean) {
    if (Test-Path $CMakeCachePath) {
      Remove-Item $CMakeCachePath -Force
    }
    if (Test-Path $CMakeFilesDir) {
      Remove-Item $CMakeFilesDir -Recurse -Force
    }
  }

  Push-Location $CppSourceDir
  try {
    cmake --preset $PresetBuild.ConfigurePreset `
      -DCMAKE_GENERATOR_INSTANCE="$VsInstallationPath"
    if ($LASTEXITCODE -ne 0) {
      throw "CMake configure failed for preset '$($PresetBuild.ConfigurePreset)' with exit code $LASTEXITCODE."
    }

    cmake --build --preset $BuildPreset
    if ($LASTEXITCODE -ne 0) {
      throw "CMake build failed for preset '$BuildPreset' with exit code $LASTEXITCODE."
    }
  }
  finally {
    Pop-Location
  }

  $BuiltExePath = Join-Path $BuildDir "$Configuration/DisplayProber.exe"
  $DestExePath = Join-Path $BinDir   "DisplayProber-$($PresetBuild.OutputArch).exe"
  $BuiltPdbPath = Join-Path $BuildDir "$Configuration/DisplayProber.pdb"
  $DestPdbPath = Join-Path $BinDir   "DisplayProber-$($PresetBuild.OutputArch).pdb"

  if (-not (Test-Path $BuiltExePath)) {
    throw "Expected build output not found: $BuiltExePath"
  }

  Copy-Item -Path $BuiltExePath -Destination $DestExePath -Force

  if (Test-Path $BuiltPdbPath) {
    Copy-Item -Path $BuiltPdbPath -Destination $DestPdbPath -Force
  }
  elseif ($Configuration -eq "Debug") {
    throw "Expected build output not found: $BuiltPdbPath"
  }
  elseif (Test-Path $DestPdbPath) {
    Remove-Item $DestPdbPath -Force
  }
}
