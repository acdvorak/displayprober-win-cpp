# Windows PowerShell v2.0 or newer
#Requires -Version 2.0

# Parent directory of this script (version.ps1).
$VerScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

. (Join-Path -Path $VerScriptRoot -ChildPath 'scripts\power_utils.ps1')

function Format-DotNetVersionList {
  param(
    [Parameter(Position = 0)]
    [DotNetRuntimeVer[]]$VersionList
  )

  if ($VersionList) {
    $rows = $VersionList |
      Format-Table -AutoSize |
      Out-String -Stream |
      Where-Object { $_ -match '\S' } |
      ForEach-Object { '    ' + $_ }
    ($rows -join "`n")
  }
  else {
    '    None found'
  }
}

$OsNameAndVer = Get-OsNameAndVersion
$PsNameAndVer = Get-PsNameAndVersion

$Classic = Format-DotNetVersionList (Get-ClassicDotNetRuntimeList)
$Modern = Format-DotNetVersionList (Get-ModernDotNetRuntimeList)

@"

Classic .NET frameworks:

$Classic

Modern .NET frameworks:

$Modern

System versions:

    - $OsNameAndVer
    - $PsNameAndVer

"@
