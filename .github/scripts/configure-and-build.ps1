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

if (-not ($archList -contains 'x86' -or $archList -contains 'x64')) {
  throw "No valid architectures were selected. Supported architectures are: x86, x64. Received: '$Architectures'."
}
$targets = @()
if ($archList -contains 'x86') {
  $targets += @{ Arch = 'x86'; CMakeArch = 'Win32' }
}
if ($archList -contains 'x64') {
  $targets += @{ Arch = 'x64'; CMakeArch = 'x64' }
}

$sourceDir = Join-Path $env:GITHUB_WORKSPACE 'src/cpp'

foreach ($target in $targets) {
  $buildDir = Join-Path $env:GITHUB_WORKSPACE "out/$($target.Arch)"
  cmake -S $sourceDir -B $buildDir -A $target.CMakeArch "-DDP4W_RELEASE_TAG=$env:VERSION_TAG"
  cmake --build $buildDir --config Debug
  cmake --build $buildDir --config Release
}
