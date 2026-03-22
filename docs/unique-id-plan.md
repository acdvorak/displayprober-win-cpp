# Plan: Stable Display Instance Identifiers (Unified)

## TL;DR

Restructure the existing 3-tier flat stable ID system into **two distinct
categories** — **port-specific** (tracks a GPU connector) and
**port-independent** (tracks a physical monitor across port swaps). The critical
implementation work is:

1. Parse EDID serial string descriptor (tag `0xFF`) from already-captured raw
   bytes — this is the key missing piece that prevents `edid_parsed_key` from
   working on monitors like the Vizio E390-B0 which report a 0xFF serial string
   but have a null 4-byte header serial.
2. Strengthen `BuildEdidParsedKey()` to use the parsed serial string as a
   fallback.
3. Restructure the JSON output from a flat priority list into two categories.
4. Add an EDID hash fingerprint as a weak port-independent fallback.
5. Add a `location_paths`-based port-specific key that survives driver
   reinstalls.

---

## Background & Analysis

### Anatomy of `monitor_device_path`

Example: `\\?\DISPLAY#SAM73A5#5&757fe5e&7&UID20737#{e6f07b5f-...}`

| Segment          | Meaning                                     | Tracks            |
| ---------------- | ------------------------------------------- | ----------------- |
| `SAM73A5`        | 3-letter EISA VID + 4-hex product code      | **Monitor model** |
| `5&757fe5e&7`    | PnP device-tree hash (from parent GPU)      | **GPU instance**  |
| `UID20737`       | Target path ID from display miniport driver | **GPU connector** |
| `{e6f07b5f-...}` | `GUID_DEVINTERFACE_MONITOR`                 | Constant          |

The full path is port-specific — the UID suffix is assigned per-connector. Two
identical monitors on the same GPU differ only in the UID. The EDID 7-char code
(e.g., `SAM73A5`) identifies the **model**, not the individual unit.

### Observed `target_path_id` Instability

Comparing the `dual_gpu_weird_phantom_disabled` and
`dual_gpu_weird_phantom_enabled` dumps: the Samsung's `target_path_id` changed
from `0x4101` to `0x400111` on the **same physical port** when a phantom display
was toggled. This means `primary_port_key` is stable under normal reboots but
**can shift** when the display topology changes (phantom displays, driver
reinstalls).

### Existing Implementation

`DisplayProberInternal.cpp` already computes three keys:

1. **`primary_port_key`** —
   `acd_ppk:gpu_id=<adapter_instance_id>;tpid=<hex(target_path_id)>` —
   PORT-SPECIFIC
2. **`monitor_path_key`** — `acd_mpk:mdp=<monitor_device_path>` — PORT-SPECIFIC
   (contains UID)
3. **`edid_parsed_key`** —
   `acd_edid:vid=<VID>;pid=<hex(PID)>;sn=<hex(serial_number_id)>` —
   PORT-INDEPENDENT (when serial exists)

These are collapsed into a single flat priority list (`stable_id_candidates`).
The elected `stable_id` is always the first non-empty candidate.

### Identified Gaps

1. **EDID serial string (tag `0xFF`) not parsed.** The `WinEdidInfo` struct has
   `serial_number_id` (the 4-byte EDID header serial at bytes 12–15) but does
   NOT parse the serial number string descriptor (tag `0xFF`). The Vizio's EDID
   contains `"LAU8PSBP01000"` in a `0xFF` descriptor while its header serial is
   null → `edid_parsed_key` is empty even though a unique serial string exists.

2. **`monitor_serial_string` field exists on `WinDisplay` but is never
   populated.** There is a TODO in `acd-json.hpp` (line ~832): _"Figure out
   which Windows API returns this value."_ — Answer: parse it from the raw EDID
   bytes already captured in `edid_bytes_base64`.

3. **`edid_parsed_key` TODO not implemented**: _"Append a hash of the full EDID
   bytes"_ (noted at `acd-json.hpp` line ~694).

4. **Output structure is flat.** `stable_id_candidates` mixes port-specific and
   port-independent identifiers without distinguishing them.

5. **No driver-reinstall-proof port key.** `primary_port_key` depends on
   `adapter_instance_id` which can change on driver reinstall. A key based on
   `location_paths` + `target_path_id` would survive this.

### Fundamental Limitation (Use Case 2)

**Two identical monitors with identical EDID data (no unique serial in either
the 4-byte header OR the string descriptor) CANNOT be distinguished after a port
swap.** This is a hardware constraint — the monitor provides no per-unit
identity to the host. No Windows API can work around this because they all
derive identity from the EDID.

`base_container_id` (DEVPKEY_Device_ContainerId from SetupAPI) is a version-5
UUID deterministically computed from EDID content — identical EDID → identical
container ID. It adds no independent value.

---

## Steps

### Phase 1: Parse EDID Descriptor Blocks from Raw Bytes _(no dependencies)_

**Step 1.** Create an EDID parser function that takes the decoded
`edid_bytes_base64` (128+ bytes) and extracts the four 18-byte descriptor blocks
at byte offsets 54, 72, 90, 108. For each descriptor: if bytes 0–2 are
`0x00 0x00 0x00`, it is a display descriptor. Byte 3 is the tag:

- `0xFF` → Serial Number String (13 ASCII chars, strip trailing `0x20`/`0x0A`)
- `0xFC` → Monitor Name String (useful for cross-validation against
  `user_friendly_name`)
- `0xFE` → Data String (sometimes contains additional model info)

**Step 2.** Populate `monitor_serial_string` on `WinDisplay` from the parsed
`0xFF` descriptor. This resolves the existing TODO at `acd-json.hpp` line ~832.

**Step 3.** Optionally populate a new `edid_data_string` field from tag `0xFE`,
if present.

### Phase 2: Strengthen `edid_parsed_key` with Serial String _(depends on Phase 1)_

**Step 4.** Modify `BuildEdidParsedKey()` to accept and incorporate the serial
string. Logic:

- `serial_number_id` non-zero → include as `sn=<hex>` (existing behavior)
- `serial_number_id` zero/null BUT `monitor_serial_string` non-empty → use
  string as `ss=<string>`
- Both available and non-trivial → include both for maximum specificity
- Neither available → return empty (cannot distinguish units of the same model)
- Proposed format: `acd_edid:vid=<VID>;pid=<hex(PID)>;sn=<hex>;ss=<string>`
  (omit `;sn=` or `;ss=` when absent)

**Step 5.** Treat `serial_number_id == 0` as "no serial assigned" — do NOT emit
`sn=0x00000000` unless the serial string is also present.

### Phase 3: Restructure Output into Two Categories _(parallel with Phase 2)_

**Step 6.** Define new JSON output structure with two identifier categories:

```
"stable_ids": {
  "port_independent": [
    { "key": "acd_edid:vid=VIZ;pid=0x1009;ss=LAU8PSBP01000", "source": "edid_parsed_key" }
  ],
  "port_specific": [
    { "key": "acd_ppk:gpu_id=...;tpid=0x00005103", "source": "primary_port_key" },
    { "key": "acd_mpk:mdp=\\?\DISPLAY#VIZ1009#...#{...}", "source": "monitor_path_key" }
  ]
}
```

Within each category, entries are ordered from strongest to weakest.

**Step 7.** Classify each key:

- `primary_port_key` → **port_specific** (contains `adapter_instance_id` +
  `target_path_id`)
- `monitor_path_key` → **port_specific** (contains UID which is connector-bound)
- `edid_parsed_key` → **port_independent** (contains only EDID-derived identity)

**Step 8.** Retain flat `stable_id`, `stable_id_candidates`, `stable_id_source`
for backward compatibility. Mark as deprecated.

### Phase 4: EDID Hash Fingerprint _(depends on Phase 1)_

**Step 9.** Compute a SHA-256 hash of the full EDID base block (first 128
bytes). Add as a `port_independent` entry with key format
`acd_edid:sha256=<hex>`. This is a **model-level** fingerprint — weaker than
serial-based `edid_parsed_key` because two identical models typically have
near-identical EDID base blocks, but catches edge cases where descriptor content
slightly differs between units (e.g., different manufacture week/year). Only
useful when serial-based `edid_parsed_key` is unavailable.

### Phase 5: Location-Based Port Key _(parallel with Phases 2–4)_

**Step 10.** Create a new `BuildLocationPortKey()` function that combines the
adapter's `location_paths` (from SetupAPI, e.g.,
`PCIROOT(0)#PCI(0200)#PCI(0000)`) with `target_path_id`. Key format:
`acd_lpk:loc=<location_path>;tpid=<hex(target_path_id)>`. This is more durable
than `primary_port_key` because `location_paths` is tied to the physical PCIe
slot, surviving driver reinstalls and registry wipes. Add as a `port_specific`
entry, ranked strongest when available.

---

## Relevant Files

- `src/cpp/DisplayProberInternal.cpp` — `BuildPrimaryPortKey()` (L68),
  `BuildMonitorPathKey()` (L82), `BuildEdidParsedKey()` (L90),
  `PopulateStableKeyFields()` (L108)
- `src/cpp/DisplayProberLib.cpp` — orchestration: key builders at L192–220,
  `PopulateStableKeyFields()` call at L359
- `src/cpp/gencode/acd-json.hpp` — `WinEdidInfo` (L402), `WinDisplay` (L696),
  `monitor_serial_string` (L836), `edid_parsed_key` TODO (L694),
  `StableIdSource` enum (L486)
- `src/cpp/SetupApiDevice.cpp` — `ReadEdidBytes()` (L279),
  `GetEdidBytesFromMonitorDevicePath()` (L333) — raw EDID already available
- `src/cpp/WmiMonitor.cpp` — `GetWinEdidInfoFromDevicePath()` (L314) — WMI-based
  EDID fields
- `src/ts/schemas/displayprober-win-cpp.schema.json` — schema definitions for
  `stable_id`, `stable_id_candidates`, `monitor_serial_string`
- `src/ts/types/displayprober-win-cpp.types.ts` — TS contract; add `stable_ids`
  type
- `src/cpp/tests/test_DisplayProberInternal.cpp` — existing stable ID unit tests
  (L29–92)
- `temp/DisplayProber-x86-dev.json` — real-world fixture with Vizio (serial
  string only) and Samsung (serial number ID only)

---

## Verification

1. **EDID parser unit tests**: Descriptor blocks with tag `0xFF`, `0xFC`,
   `0xFE`; missing descriptors; malformed/short EDID; padding removal.
2. **`BuildEdidParsedKey()` unit tests**: Serial string only (Vizio case),
   serial ID only (Samsung case), both present, neither present. Verify
   `serial_number_id == 0` treated as absent.
3. **Integration with existing dumps**: Re-process the Vizio dump → verify
   `edid_parsed_key` now contains `ss=LAU8PSBP01000`. Samsung retains its
   existing `edid_parsed_key` with `sn=0x01004400`.
4. **Dual-identical-model synthetic test**: Two monitors with same VID+PID but
   different serial strings → distinct `edid_parsed_key`. Same VID+PID and NO
   serial → `edid_parsed_key` empty for both.
5. **JSON structure test**: Verify `stable_ids.port_specific` and
   `stable_ids.port_independent` appear alongside deprecated flat fields.
6. **Cross-reference**: Compare parsed `monitor_serial_string` values against
   NirSoft `MultiMonitorTool.xml` and `ControlMyMonitor.txt` in existing dump
   directories.
7. **Reboot test**: Unchanged cabling — all keys remain identical.
8. **Swap test** (if two identical monitors available): Same port-independent
   identity, changed port-specific identity.

---

## Decisions

- **EDID serial string source**: Parse from raw EDID bytes in
  `edid_bytes_base64` (tag `0xFF` descriptor). No new Windows API needed — the
  raw bytes are already captured.
- **Identical-EDID limitation**: Documented as a known, unsolvable hardware
  constraint. No DDC/CI investigation.
- **`serial_number_id == 0`**: Treated as "no serial" (same as null) to avoid
  false matches between units that both report 0.
- **`base_container_id`**: NOT promoted as an identifier. It is derived from the
  same EDID data and adds no independent discriminating value.
- **`monitor_path_key` classification**: Port-specific (not port-independent)
  because it contains the UID suffix which is connector-bound.
- **Output structure**: Two categories (`port_specific`, `port_independent`),
  keeping old flat fields for backward compatibility.
- **Excluded**: Persistence/lifecycle management, confidence scoring, weighted
  resolvers, swap detection logic — these are beyond the stated goal of
  computing stable IDs.
- **Excluded**: DDC/CI serial probes, USB hub IDs — not needed for this phase.
