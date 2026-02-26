# Windows PowerShell v5.1
#Requires -Version 5.1
#Requires -PSEdition Desktop

$InformationPreference = 'Continue'

$installUserScript = Join-Path $PSScriptRoot "scripts/install_user.ps1"
$installAdminScript = Join-Path $PSScriptRoot "scripts/install_admin.ps1"

if (-not (Test-Path $installUserScript)) {
  throw "Missing script: '$installUserScript'"
}

if (-not (Test-Path $installAdminScript)) {
  throw "Missing script: '$installAdminScript'"
}

Write-Host ""
Write-Host "Running install_user.ps1..."
Write-Host ""
& $installUserScript
if ($LASTEXITCODE -ne 0) {
  Write-Host ""
  throw "install_user.ps1 failed with exit code $LASTEXITCODE."
}
Write-Host ""

Write-Host ""
Write-Host "Running install_admin.ps1..."
Write-Host ""
& $installAdminScript
if ($LASTEXITCODE -ne 0) {
  throw "install_admin.ps1 failed with exit code $LASTEXITCODE."
}
Write-Host ""

Write-Host ""
Write-Host "Dependency installation completed successfully."
Write-Host ""
