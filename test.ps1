# Windows PowerShell v5.1
#Requires -Version 5.1
#Requires -PSEdition Desktop

$InformationPreference = 'Continue'
$ErrorActionPreference = 'Stop'

$CppSourceDir = Join-Path $PSScriptRoot 'src/cpp'
$CMakePresetsPath = Join-Path $CppSourceDir 'CMakePresets.json'

if (-not (Test-Path $CMakePresetsPath)) {
  throw "CMakePresets.json was not found at: $CMakePresetsPath"
}

Push-Location $CppSourceDir
try {
  cmake --preset x64-tests
  if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed for preset 'x64-tests' with exit code $LASTEXITCODE."
  }

  cmake --build --preset x64-Debug-Tests
  if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed for preset 'x64-Debug-Tests' with exit code $LASTEXITCODE."
  }

  ctest --preset x64-Debug-Tests
  if ($LASTEXITCODE -ne 0) {
    throw "CTest failed for preset 'x64-Debug-Tests' with exit code $LASTEXITCODE."
  }
}
finally {
  Pop-Location
}
