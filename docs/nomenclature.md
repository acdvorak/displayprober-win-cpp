"Adapter":

In the Display Config (`QueryDisplayConfig`/`SetDisplayConfig`) world, an
"adapter" is not "a monitor" and not even strictly "a physical GPU". It is the
OS-visible display adapter instance that owns the endpoints in the topology -
the thing that provides:

- sources (scan-out engines that can produce a desktop surface)
- targets (connectors/sinks the OS can route a source to)

Every `DISPLAYCONFIG_PATH_*` struct points at an adapter via an adapterId, which
is a LUID (locally unique identifier). That LUID identifies the runtime instance
of the adapter (it can change across reboot, or if the PnP device is
stopped/started).

So what can an "adapter" be besides a physical GPU?

- Integrated GPU (iGPU) vs discrete GPU (dGPU)
  - If both are enabled, you can see multiple adapter LUIDs, each owning
    different sources/targets.
- A "basic" / fallback display adapter
  - Example: Microsoft Basic Display Adapter when proper vendor drivers are
    missing (still a real WDDM display adapter as far as the OS is concerned).
- A virtual or indirect display adapter provided by a driver
  - USB graphics (for example, DisplayLink-style solutions) and other "virtual
    monitor" solutions use an Indirect Display Driver model: they create a
    display adapter instance that can expose targets even though there is no
    physical GPU connector.
- Wireless display / projection paths (Miracast and similar)
  - Windows can represent these as display-capable paths managed through the
    display driver stack, so they show up as targets owned by an adapter in the
    topology.
- A synthetic adapter in a VM / remote environment
  - In virtual machines you typically have a virtual display adapter; in some
    remote scenarios Windows may also present virtualized display paths.

Mental model that usually keeps you sane:

- "Adapter" (LUID) = "which display driver instance owns this source/target?"
- "Source id" = "which scan-out/source on that adapter?"
- "Target id" = "which connector/sink endpoint on that adapter?"
