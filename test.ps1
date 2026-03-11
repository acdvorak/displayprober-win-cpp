#Requires -Version 5.1

$InformationPreference = 'Continue'
$ErrorActionPreference = 'Stop'

$CppSourceDir = Join-Path $PSScriptRoot 'src/cpp'
$CMakePresetsPath = Join-Path $CppSourceDir 'CMakePresets.json'

if (-not (Test-Path $CMakePresetsPath)) {
  throw "CMakePresets.json was not found at: $CMakePresetsPath"
}

Push-Location $CppSourceDir
try {
  cmake --preset Win32-Tests
  if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed for preset 'Win32-Tests' with exit code $LASTEXITCODE."
  }

  cmake --build --preset Win32-Debug-Tests
  if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed for preset 'Win32-Debug-Tests' with exit code $LASTEXITCODE."
  }

  ctest --preset Win32-Debug-Tests
  if ($LASTEXITCODE -ne 0) {
    throw "CTest failed for preset 'Win32-Debug-Tests' with exit code $LASTEXITCODE."
  }
}
finally {
  Pop-Location
}
