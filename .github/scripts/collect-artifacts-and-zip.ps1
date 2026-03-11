#Requires -Version 7

param(
  [string]$Architectures = $env:ARCHITECTURES
)

$InformationPreference = 'Continue'

if ([string]::IsNullOrWhiteSpace($Architectures)) {
  $Architectures = 'x86,x64'
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
$binRoot = Join-Path $env:GITHUB_WORKSPACE 'bin'
$debugDir = Join-Path $binRoot 'Debug'
$releaseDir = Join-Path $binRoot 'Release'
$zipRoot = Join-Path $env:GITHUB_WORKSPACE 'zip'

New-Item -ItemType Directory -Path $debugDir -Force | Out-Null
New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null
New-Item -ItemType Directory -Path $zipRoot -Force | Out-Null

foreach ($arch in $archList) {
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
