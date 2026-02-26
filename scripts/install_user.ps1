# Windows PowerShell v5.1
#Requires -Version 5.1
#Requires -PSEdition Desktop

$InformationPreference = 'Continue'

################################################################################
# Install winget (current user)
################################################################################

$winget = Get-Command winget.exe -ErrorAction SilentlyContinue
if ($null -eq $winget) {
  Write-Host "winget not found. Installing App Installer for current user..."

  $wingetBundlePath = Join-Path $env:TEMP "Microsoft.DesktopAppInstaller.msixbundle"
  try {
    Invoke-WebRequest "https://aka.ms/getwinget" -OutFile $wingetBundlePath -UseBasicParsing -ErrorAction Stop
  } catch {
    throw "Failed to download winget bootstrap package. Check your internet connection or download App Installer manually from Microsoft Store and re-run this script. Details: $($_.Exception.Message)"
  }

  try {
    Add-AppxPackage -Path $wingetBundlePath -ErrorAction Stop
  } catch {
    throw "Failed to install winget for current user. Install App Installer manually from Microsoft Store and re-run this script. Details: $($_.Exception.Message)"
  }

  $winget = Get-Command winget.exe -ErrorAction SilentlyContinue
  if ($null -eq $winget) {
    $windowsAppsPath = Join-Path $env:LOCALAPPDATA "Microsoft\WindowsApps"
    if (Test-Path $windowsAppsPath) {
      $env:Path = "$windowsAppsPath;$env:Path"
    }
    $winget = Get-Command winget.exe -ErrorAction SilentlyContinue
  }

  if ($null -eq $winget) {
    throw "winget was installed, but winget.exe is still unavailable in this session. Open a new PowerShell and re-run install_user.ps1."
  }
}

Write-Host "winget detected at '$($winget.Source)'."
& $winget.Source --version
if ($LASTEXITCODE -ne 0) {
  throw "winget was found but failed to run."
}

Write-Host ""

################################################################################
# Install CMake (current user)
################################################################################

$cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue

if ($null -eq $cmakeCommand) {
  Write-Host "CMake not found. Installing via winget for current user..."

  $wingetArgs = @(
    "install",
    "--id", "Kitware.CMake",
    "--exact",
    "--silent",
    "--accept-package-agreements",
    "--accept-source-agreements",
    "--scope", "user"
  )

  $wingetProcess = Start-Process -FilePath $winget.Source -ArgumentList $wingetArgs -Wait -PassThru
  if ($wingetProcess.ExitCode -ne 0) {
    throw "winget failed to install CMake for current user (exit code $($wingetProcess.ExitCode))."
  }

  # Refresh PATH in the current session so cmake can be resolved immediately.
  $processPath = [Environment]::GetEnvironmentVariable("Path", "Process")
  $userPath    = [Environment]::GetEnvironmentVariable("Path", "User")
  $machinePath = [Environment]::GetEnvironmentVariable("Path", "Machine")
  $env:Path    =
    (
      @($processPath, $userPath, $machinePath) |
        Where-Object   { -not [string]::IsNullOrWhiteSpace($_) } |
        ForEach-Object { $_.TrimEnd(';') } |
        Where-Object   { $_ -ne '' }
    ) -join ';'
}

$cmake = Get-Command cmake.exe -ErrorAction SilentlyContinue
if ($null -eq $cmake) {
  throw "CMake installation appears to have completed, but cmake.exe is still not found in PATH."
}

Write-Host "CMake detected at '$($cmake.Source)'."
& $cmake.Source --version
if ($LASTEXITCODE -ne 0) {
  throw "CMake was found but failed to run."
}

Write-Host ""

################################################################################
# Finish
################################################################################

Write-Host ""
Write-Host "User-scope dependencies verification succeeded."
