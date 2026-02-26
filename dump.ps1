# Nothing over SSH.
..\sysinfo-dump\bin\dumpedid\DumpEDID.exe -a > .\temp\DumpEDID.txt

# Nothing over SSH.
..\sysinfo-dump\bin\monitorinfoview\MonitorInfoView.exe /HideInactiveMonitors 1 /sxml .\temp\MonitorInfoView.xml

# Nothing over SSH.
..\sysinfo-dump\bin\multimonitortool\MultiMonitorTool.exe /HideInactiveMonitors 1 /sxml .\temp\MultiMonitorTool.xml

# Nothing over SSH.
..\sysinfo-dump\bin\controlmymonitor\ControlMyMonitor.exe /smonitors .\temp\ControlMyMonitor.txt

# Over SSH, even when physical displays are connected, this returns
# `"DeviceName": "WinDisc"` and 1024x768 by default.
Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.Screen]::AllScreens > .\temp\WinFormsScreens.txt

# Works even over SSH.
Get-CimInstance Win32_VideoController > .\temp\Win32_VideoController.txt

# Works even over SSH.
Get-CimInstance Win32_PnPEntity | Where-Object { $_.PNPClass -in @('Monitor', 'Display') } > .\temp\Win32_PnPEntity.txt

# Works even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorBasicDisplayParams > .\temp\WmiMonitorBasicDisplayParams.txt

# Works even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorConnectionParams > .\temp\WmiMonitorConnectionParams.txt

# Works even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorDescriptorMethods > .\temp\WmiMonitorDescriptorMethods.txt

# Works even over SSH.
Get-CimInstance -Namespace root\wmi WmiMonitorID > .\temp\WmiMonitorID.txt
