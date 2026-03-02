#Requires -Version 5.1

$InformationPreference = 'Continue'

function Confirm-VSToolchain {
  $vsDevCmdPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat'
  if (-not (Test-Path $vsDevCmdPath)) {
    throw "VsDevCmd.bat was not found at '$vsDevCmdPath'."
  }

  $validationCommand = "`"$vsDevCmdPath`" -arch=x64 && where cl && where link && where rc"
  cmd /c $validationCommand
  if ($LASTEXITCODE -ne 0) {
    throw 'VC tools installed, but Windows SDK resource compiler (rc.exe) is unavailable. Ensure Windows 10 SDK is installed in VS Build Tools Installer.'
  }

  Write-Host ''
  Write-Host 'Visual Studio Build Tools verification succeeded (cl/link/rc found).'
  Write-Host ''
}

################################################################################
# Elevate to Admin
################################################################################

$currentIdentity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($currentIdentity)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
  $scriptPath = $MyInvocation.MyCommand.Path
  $elevated = Start-Process -FilePath 'powershell.exe' -Verb RunAs -Wait -PassThru -ArgumentList @(
    '-NoProfile',
    '-ExecutionPolicy', 'Bypass',
    '-File', "`"$scriptPath`""
  )

  Write-Host ''

  Confirm-VSToolchain

  exit $elevated.ExitCode
}

################################################################################
# Install VS 2022 Build Tools and Windows SDKs
################################################################################

try {
  $buildToolsExe = Join-Path $PSScriptRoot '../src/cpp/vs_BuildTools.exe'
  $vsConfigPath = Join-Path $PSScriptRoot '../src/cpp/.vsconfig'

  if (-not (Test-Path $buildToolsExe)) {
    Write-Host 'Downloading VS 2022 Build Tools installer...'
    Invoke-WebRequest 'https://aka.ms/vs/17/release/vs_BuildTools.exe' -OutFile $buildToolsExe -UseBasicParsing -ErrorAction Stop
  }

  $arguments = @(
    '--config', $vsConfigPath,
    '--passive',  # Show progress UI; no user input needed
    '--wait',     # Block the shell until the installation completes
    '--norestart' # Don't prompt the user to restart
  )

  Write-Host 'Installing minimal VS 2022 Build Tools:'

  if (-not (Test-Path $vsConfigPath)) {
    throw ".vsconfig was not found at '$vsConfigPath'."
  }

  try {
    $vsConfig = Get-Content -Path $vsConfigPath -Raw -ErrorAction Stop | ConvertFrom-Json
  }
  catch {
    throw "Failed to parse .vsconfig at '$vsConfigPath': $($_.Exception.Message)"
  }

  $components = @($vsConfig.components | Where-Object { $_ -is [string] -and -not [string]::IsNullOrWhiteSpace($_) })
  foreach ($component in $components) {
    Write-Host "  - $component"
  }

  Write-Host ''
  Write-Host 'This may take several minutes...'
  Write-Host ''

  $process = Start-Process -FilePath $buildToolsExe -ArgumentList $arguments -Wait -PassThru
  if ($process.ExitCode -ne 0) {
    throw "vs_BuildTools.exe failed with exit code $($process.ExitCode)."
  }

  ################################################################################
  # Validate toolchain
  ################################################################################

  Confirm-VSToolchain
}
catch {
  Write-Error $_
  Write-Host ''
  [void](Read-Host 'An error occurred while running elevated. Press ENTER to exit')
  exit 1
}
