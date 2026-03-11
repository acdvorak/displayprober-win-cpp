#Requires -Version 5.1

$InformationPreference = 'Continue'

################################################################################
# Install winget (current user)
################################################################################

$winget = Get-Command winget.exe -ErrorAction SilentlyContinue
if ($null -eq $winget) {
  Write-Host 'winget not found. Installing App Installer for current user...'

  $wingetBundlePath = Join-Path $env:TEMP 'Microsoft.DesktopAppInstaller.msixbundle'
  try {
    Invoke-WebRequest 'https://aka.ms/getwinget' -OutFile $wingetBundlePath -UseBasicParsing -ErrorAction Stop
  }
  catch {
    throw "Failed to download winget bootstrap package. Check your internet connection or download App Installer manually from Microsoft Store and re-run this script. Details: $($_.Exception.Message)"
  }

  try {
    Add-AppxPackage -Path $wingetBundlePath -ErrorAction Stop
  }
  catch {
    throw "Failed to install winget for current user. Install App Installer manually from Microsoft Store and re-run this script. Details: $($_.Exception.Message)"
  }

  $winget = Get-Command winget.exe -ErrorAction SilentlyContinue
  if ($null -eq $winget) {
    $windowsAppsPath = Join-Path $env:LOCALAPPDATA 'Microsoft\WindowsApps'
    if (Test-Path $windowsAppsPath) {
      $env:Path = "$windowsAppsPath;$env:Path"
    }
    $winget = Get-Command winget.exe -ErrorAction SilentlyContinue
  }

  if ($null -eq $winget) {
    throw 'winget was installed, but winget.exe is still unavailable in this session. Open a new PowerShell and re-run install_user.ps1.'
  }
}

Write-Host "winget detected at '$($winget.Source)'."

& $winget.Source --version

if ($LASTEXITCODE -ne 0) {
  throw 'winget was found but failed to run.'
}

################################################################################
# Install NuGet (current user)
################################################################################

$winget.Source install -e --id Microsoft.NuGet --scope user

if ($LASTEXITCODE -ne 0) {
  throw "Failed to install NuGet (Microsoft.NuGet) for current user via winget. Exit code: $LASTEXITCODE"
}

################################################################################
# Finish
################################################################################

Write-Host ''
Write-Host 'User-scope dependencies verification succeeded.'
