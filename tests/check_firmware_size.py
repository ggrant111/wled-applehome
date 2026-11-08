#!/usr/bin/env python3
"""Hard OTA partition-size gate; fail before offering an overfilled firmware.bin."""
from pathlib import Path
import sys

OTA_SLOT_BYTES = 0x1F0000   # partitions.csv: 2,031,616 bytes
MIN_HEADROOM_BYTES = 70000  # reserve ~68KiB for future fixes

if len(sys.argv) != 2:
    raise SystemExit("Usage: python tests/check_firmware_size.py path/to/firmware.bin")
path = Path(sys.argv[1])
if not path.is_file():
    raise SystemExit(f"Missing firmware: {path}")
size = path.stat().st_size
headroom = OTA_SLOT_BYTES - size
print(f"firmware={size:,} bytes / OTA slot={OTA_SLOT_BYTES:,} "
      f"({size / OTA_SLOT_BYTES * 100:.1f}%), remaining={headroom:,} bytes")
if headroom < MIN_HEADROOM_BYTES:
    raise SystemExit("FAIL: firmware too close to OTA partition capacity. "
                     "Shrink app or revise hardware/partition plan before release.")
print("PASS: OTA headroom guard")
