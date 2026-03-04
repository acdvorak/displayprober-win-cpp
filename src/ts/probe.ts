import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

import { $, usePowerShell } from 'zx';
import { sortJsonc, type CompareFn } from 'sort-jsonc';

import type {
  WinDisplayProberJson,
  WinDisplay,
  WinScreenRectangle,
  WinStandardColorInfo,
  WinAdvancedColorInfo,
  WinEdidInfo,
} from './types/displayprober-win-cpp.types';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT_DIR = resolve(__dirname, '../../');
const EXE_PATH = resolve(ROOT_DIR, 'bin/DisplayProber-x64.exe');

type Key =
  | keyof WinDisplayProberJson
  | keyof WinDisplay
  | keyof WinScreenRectangle
  | keyof WinStandardColorInfo
  | keyof WinAdvancedColorInfo
  | keyof WinEdidInfo;

const DISPLAY_KEYS: ReadonlyArray<keyof WinDisplay> = [
  'friendly_name',
  'short_lived_identifier',

  'is_primary',
  'is_attached_to_desktop',

  'physical_connector_type',
  'refresh_rate_hz',
  'refresh_rate_numerator',
  'refresh_rate_denominator',

  'rotation_deg',
  'scan_line_ordering',
  'dpi_scaling_percent',

  'standard_color_info',
  'advanced_color_info',
  'edid_info',

  'bounds',
  'working_area',

  'stable_id',
  'stable_id_source',
  'stable_id_candidates',
  'adapter_device_path',
  'adapter_instance_id',
  'monitor_device_path',
  'monitor_path_key',
  'primary_port_key',
  'target_path_id',
];

const RECTANGLE_KEYS: ReadonlyArray<keyof WinScreenRectangle> = [
  'x',
  'y',
  'width',
  'height',
  'top',
  'bottom',
  'left',
  'right',
];

const STANDARD_COLOR_KEYS: ReadonlyArray<keyof WinStandardColorInfo> = [
  'is_hdr_supported',
  'is_hdr_enabled',

  'bits_per_channel',
  'color_encoding',
  'dxgi_color_space',

  'min_luminance_nits',
  'max_luminance_nits',
  'max_full_frame_luminance_nits',
];

const ADVANCED_COLOR_KEYS: ReadonlyArray<keyof WinAdvancedColorInfo> = [
  'active_color_mode',

  'is_advanced_color_supported',
  'is_advanced_color_active',
  'is_advanced_color_enabled',
  'is_advanced_color_limited_by_policy',
  'is_advanced_color_force_disabled',

  'is_wide_color_supported',
  'is_wide_color_user_enabled',
  'is_wide_color_enforced',

  'is_high_dynamic_range_supported',
  'is_high_dynamic_range_user_enabled',
];

const EDID_KEYS: ReadonlyArray<keyof WinEdidInfo> = [
  'user_friendly_name',
  'manufacturer_vid',
  'product_code_id',
  'serial_number_id',
  'year_of_manufacture',
  'week_of_manufacture',
  'video_output_technology_type',
  'wmi_instance_name',
  'wmi_join_key',
  'monitor_device_path',
  'max_horizontal_image_size_mm',
  'max_vertical_image_size_mm',
];

const PRIORITY_KEYS: readonly Key[] = [
  //
  ...DISPLAY_KEYS,
  ...RECTANGLE_KEYS,
  ...STANDARD_COLOR_KEYS,
  ...ADVANCED_COLOR_KEYS,
  ...EDID_KEYS,
];

const priorityIndex = new Map<Key, number>(
  PRIORITY_KEYS.map((key, index) => [key, index] as const),
);

const jsonComparator = ((key1: Key, key2: Key): number => {
  const index1 = priorityIndex.get(key1);
  const index2 = priorityIndex.get(key2);

  if (index1 !== undefined && index2 !== undefined) {
    return index1 - index2;
  }
  if (index1 !== undefined) {
    return -1;
  }
  if (index2 !== undefined) {
    return 1;
  }

  return key1.localeCompare(key2, 'en-us');
}) as CompareFn;

async function main(): Promise<void> {
  // Windows PowerShell v5.1
  usePowerShell();

  const cmd = await $`& ${EXE_PATH}`;
  const stdout = cmd.stdout;

  const sorted = sortJsonc(stdout, {
    sort: jsonComparator,
    spaces: 2,
  });

  console.log(sorted);
}

main();
