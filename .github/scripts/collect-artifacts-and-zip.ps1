#Requires -Version 7

param(
  [string]$Architectures = $env:ARCHITECTURES,
  [string]$BuildTypes = $env:BUILD_TYPES
)

$InformationPreference = 'Continue'

if ([string]::IsNullOrWhiteSpace($Architectures)) {
  $Architectures = 'x86,x64'
}
if ([string]::IsNullOrWhiteSpace($BuildTypes)) {
  $BuildTypes = 'Debug,Release'
}

$archList = $Architectures -split ',' |
  ForEach-Object { $_.Trim() } |
  Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

if (-not $archList -or $archList.Count -eq 0) {
  throw "No valid architectures specified. Input value: '$Architectures'"
}

$supportedArchitectures = @('x86', 'x64', 'arm64')
$invalidArchitectures = $archList | Where-Object { $supportedArchitectures -notcontains $_ }
if ($invalidArchitectures -and $invalidArchitectures.Count -gt 0) {
  $invalidList = ($invalidArchitectures | Sort-Object -Unique) -join ', '
  $supportedList = $supportedArchitectures -join ', '
  throw "Unsupported architecture(s) specified: $invalidList. Supported architectures are: $supportedList."
}

$configList = $BuildTypes -split ',' |
  ForEach-Object { $_.Trim() } |
  Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

if (-not ($configList -contains 'Debug' -or $configList -contains 'Release')) {
  throw "No valid build types were selected. Supported build types are: Debug, Release. Received: '$BuildTypes'."
}

$binRoot = Join-Path $env:GITHUB_WORKSPACE 'bin'
$debugDir = Join-Path $binRoot 'Debug'
$releaseDir = Join-Path $binRoot 'Release'
$zipRoot = Join-Path $env:GITHUB_WORKSPACE 'zip'

if ($configList -contains 'Debug') {
  New-Item -ItemType Directory -Path $debugDir -Force | Out-Null
}
if ($configList -contains 'Release') {
  New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null
}
New-Item -ItemType Directory -Path $zipRoot -Force | Out-Null

foreach ($arch in $archList) {
  $buildRoot = Join-Path $env:GITHUB_WORKSPACE "out/$arch"

  if ($configList -contains 'Debug') {
    $buildDebugDir = Join-Path $buildRoot 'Debug'
    $sourceDebugExe = Join-Path $buildDebugDir 'DisplayProber.exe'
    $sourceDebugPdb = Join-Path $buildDebugDir 'DisplayProber.pdb'

    foreach ($requiredPath in @($sourceDebugExe, $sourceDebugPdb)) {
      if (-not (Test-Path $requiredPath)) {
        throw "Expected build artifact not found: $requiredPath"
      }
    }

    $destDebugExe = Join-Path $debugDir "DisplayProber-$env:VERSION_TAG-$arch.exe"
    $destDebugPdb = Join-Path $debugDir "DisplayProber-$env:VERSION_TAG-$arch.pdb"
    Copy-Item -LiteralPath $sourceDebugExe -Destination $destDebugExe -Force
    Copy-Item -LiteralPath $sourceDebugPdb -Destination $destDebugPdb -Force
  }

  if ($configList -contains 'Release') {
    $buildReleaseDir = Join-Path $buildRoot 'Release'
    $sourceReleaseExe = Join-Path $buildReleaseDir 'DisplayProber.exe'

    foreach ($requiredPath in @($sourceReleaseExe)) {
      if (-not (Test-Path $requiredPath)) {
        throw "Expected build artifact not found: $requiredPath"
      }
    }

    $destReleaseExe = Join-Path $releaseDir "DisplayProber-$env:VERSION_TAG-$arch.exe"
    Copy-Item -LiteralPath $sourceReleaseExe -Destination $destReleaseExe -Force
  }
}

$debugZip = Join-Path $zipRoot "DisplayProber-$env:VERSION_TAG-debug.zip"
$releaseZip = Join-Path $zipRoot "DisplayProber-$env:VERSION_TAG-release.zip"

if (Test-Path $debugZip) {
  Remove-Item $debugZip -Force
}
if (Test-Path $releaseZip) {
  Remove-Item $releaseZip -Force
}

if ($configList -contains 'Debug') {
  Compress-Archive -Path (Join-Path $debugDir '*') -DestinationPath $debugZip
}
if ($configList -contains 'Release') {
  Compress-Archive -Path (Join-Path $releaseDir '*') -DestinationPath $releaseZip
}
