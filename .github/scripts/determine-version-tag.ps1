# Modern PowerShell v7+
#Requires -Version 7
#Requires -PSEdition Core

$InformationPreference = 'Continue'

$releaseTag = $env:RELEASE_TAG_INPUT

if (-not $releaseTag) {
  $cmakeLists = Get-Content -Raw "$env:GITHUB_WORKSPACE/src/cpp/CMakeLists.txt"
  $match = [regex]::Match($cmakeLists, 'project\("DisplayProber"\s+VERSION\s+"([^"]+)"')
  if (-not $match.Success) {
    throw "Unable to read project version from src/cpp/CMakeLists.txt"
  }
  $releaseTag = "v$($match.Groups[1].Value)"
}

if (-not $releaseTag.StartsWith("v")) {
  $releaseTag = "v$releaseTag"
}

"VERSION_TAG=$releaseTag" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
"version_tag=$releaseTag" | Out-File -FilePath $env:GITHUB_OUTPUT -Encoding utf8 -Append
