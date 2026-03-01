## 1. Outline of strategy

Given your stated goal ("persist across reboots if possible, and stable as long
as monitors stay on the same ports"), your best key is usually NOT (`LUID`,
`targetId`). It is something like:

- Adapter persistent id (PCI location path or PnP device instance id) + targetId

And on the monitor side, for a fallback when ports move:

- `monitorDevicePath` (from `DISPLAYCONFIG_TARGET_DEVICE_NAME`) and/or EDID
  identity.

Concrete approach that matches your intent:

- Primary key (port-sticky):
  - adapter PnP instance id (or PCI bus/device/function) + `targetInfo.id`
- Secondary key (monitor-sticky, for when the same monitor moves ports but you
  still want to match it):
  - EDID (mfg/product/serial) when serial exists, else `monitorDevicePath` as
    best-effort

You will need a string component for the adapter instance id (and possibly
`monitorDevicePath`). For example:

- "gpu=" + AdapterInstanceId + ":tgt=" + Hex(targetId)

---

## 2. Matching algorithm (this is where "works in practice" happens)

When enumerating displays at startup, compute a list of candidate keys for each
current display, ordered strongest to weakest:

1. PrimaryPortKey (adapterDevicePath + targetId)
2. MonitorPathKey (monitorDevicePath)
3. EdidKey (if you have it)

Then resolve preferences like this:

- First match wins in that priority order.
- If you match via (2) or (3) but (1) is available, migrate:
- Attach the stored prefs to the new PrimaryPortKey
- Keep the old key as an alias (optional) so you can keep migrating across
  future changes

This gives you:

- Stable names as long as monitors stay on same ports
- Reasonable recovery if something changed (driver reinstall, GPU change, etc.)

## 3. How to build your "display objects" with your existing APIs

You already have 3 enumerations:

- WinUser/GDI monitor rectangles: `EnumDisplayMonitors` + `GetMonitorInfoW`
  (gives `HMONITOR`, bounds, work area)
- DisplayConfig paths: `QueryDisplayConfig` + `DisplayConfigGetDeviceInfo`
- DXGI outputs: `EnumAdapters`/`EnumOutputs`/`GetDesc1` (gives `HMONITOR` via
  `DXGI_OUTPUT_DESC` / `DESC1`)

Use `HMONITOR` as the glue:

- `DXGI_OUTPUT_DESC(1).Monitor` is an `HMONITOR`
- `EnumDisplayMonitors` gives `HMONITOR` So you can unify "this output" across
  WinUser and DXGI.

For `DisplayConfig`, the trick is:

- Use `DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME` for each sourceInfo
  (`adapterId` + `sourceId`) to get the GDI device name (like `"\.\DISPLAY1"`)
- Then map that to WinUser via `GetMonitorInfoW(szDevice)` or
  `EnumDisplayDevices`, depending on how you structure it

But for port keying you do not need DXGI at all. You only need `DisplayConfig`
target side.
