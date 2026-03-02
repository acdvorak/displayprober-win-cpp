#Requires -Version 7

$InformationPreference = 'Continue'

$targets = @(
  @{ Arch = 'x86'; CMakeArch = 'Win32' },
  @{ Arch = 'x64'; CMakeArch = 'x64' }
)

$sourceDir = Join-Path $env:GITHUB_WORKSPACE 'src/cpp'

foreach ($target in $targets) {
  $buildDir = Join-Path $env:GITHUB_WORKSPACE "out/$($target.Arch)"
  cmake -S $sourceDir -B $buildDir -A $target.CMakeArch "-DDP4W_RELEASE_TAG=$env:VERSION_TAG"
  cmake --build $buildDir --config Debug
  cmake --build $buildDir --config Release
}
