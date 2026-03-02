#Requires -Version 7

$InformationPreference = 'Continue'

$binRoot = Join-Path $env:GITHUB_WORKSPACE 'bin'
$debugDir = Join-Path $binRoot 'Debug'
$releaseDir = Join-Path $binRoot 'Release'
$zipRoot = Join-Path $env:GITHUB_WORKSPACE 'zip'

New-Item -ItemType Directory -Path $debugDir -Force | Out-Null
New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null
New-Item -ItemType Directory -Path $zipRoot -Force | Out-Null

foreach ($arch in @('x86', 'x64')) {
  $buildRoot = Join-Path $env:GITHUB_WORKSPACE "out/$arch"
  $buildDebugDir = Join-Path $buildRoot 'Debug'
  $buildReleaseDir = Join-Path $buildRoot 'Release'

  $sourceDebugExe = Join-Path $buildDebugDir 'DisplayProber.exe'
  $sourceDebugPdb = Join-Path $buildDebugDir 'DisplayProber.pdb'
  $sourceReleaseExe = Join-Path $buildReleaseDir 'DisplayProber.exe'

  foreach ($requiredPath in @($sourceDebugExe, $sourceDebugPdb, $sourceReleaseExe)) {
    if (-not (Test-Path $requiredPath)) {
      throw "Expected build artifact not found: $requiredPath"
    }
  }

  $destDebugExe = Join-Path $debugDir "DisplayProber-$env:VERSION_TAG-$arch.exe"
  $destDebugPdb = Join-Path $debugDir "DisplayProber-$env:VERSION_TAG-$arch.pdb"
  $destReleaseExe = Join-Path $releaseDir "DisplayProber-$env:VERSION_TAG-$arch.exe"

  Copy-Item -LiteralPath $sourceDebugExe -Destination $destDebugExe -Force
  Copy-Item -LiteralPath $sourceDebugPdb -Destination $destDebugPdb -Force
  Copy-Item -LiteralPath $sourceReleaseExe -Destination $destReleaseExe -Force
}

$debugZip = Join-Path $zipRoot "DisplayProber-$env:VERSION_TAG-debug.zip"
$releaseZip = Join-Path $zipRoot "DisplayProber-$env:VERSION_TAG-release.zip"

if (Test-Path $debugZip) {
  Remove-Item $debugZip -Force
}
if (Test-Path $releaseZip) {
  Remove-Item $releaseZip -Force
}

Compress-Archive -Path (Join-Path $debugDir '*') -DestinationPath $debugZip
Compress-Archive -Path (Join-Path $releaseDir '*') -DestinationPath $releaseZip
