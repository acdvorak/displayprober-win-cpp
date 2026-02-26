# Windows PowerShell v2.0 or newer
#Requires -Version 2.0

$os_obj = Get-WmiObject Win32_OperatingSystem
$os_name = $os_obj.Caption.Trim() -replace '^Microsoft ', ''
$sp = if ($os_obj.ServicePackMajorVersion -gt 0) {
  ' SP{0}' -f $os_obj.ServicePackMajorVersion
}
else {
  ''
}

$os_full = '{0}{1} (build {2})' -f $os_name, $sp, $os_obj.BuildNumber
$ps_full = 'PowerShell v{0}.{1}' -f $PSVersionTable.PSVersion.Major, $PSVersionTable.PSVersion.Minor

@"

$os_full
$ps_full

"@
