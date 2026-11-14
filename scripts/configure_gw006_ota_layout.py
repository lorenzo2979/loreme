#!/usr/bin/env python3
"""Patch the PINNED AmebaD build-source OTA layout for the actual GW006 dump.

Only edits the vendor SDK checkout on CI. No device I/O or flashing.
The observed OTA2 section starts at 0x286000 in the supplied 8MiB backup,
not at the pinned SDK default offset 0x106000.
This is necessary but INSUFFICIENT to approve an installable image.
"""
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path

BOOTCFG = Path("component/soc/realtek/amebad/fwlib/usrcfg/rtl8721d_bootcfg.c")
OTAH = Path("component/soc/realtek/amebad/fwlib/include/rtl8721d_ota.h")
OTA1 = "0x08006000"
SDK_OTA2 = "0x08106000"
GW006_OTA2 = "0x08286000"

class LayoutError(ValueError):
    pass

def configure(sdk: Path, apply: bool) -> dict:
    sdk = sdk.resolve(strict=True)
    cfg = (sdk / BOOTCFG).resolve(strict=True)
    hdr = (sdk / OTAH).resolve(strict=True)
    if not cfg.is_relative_to(sdk) or not hdr.is_relative_to(sdk):
        raise LayoutError("SDK source escapes verified checkout")
    old_cfg, old_hdr = cfg.read_text(), hdr.read_text()
    cfg_anchor = re.search(
        r"u32\s+OTA_Region\s*\[\s*2\s*\]\s*=\s*\{"
        r"\s*0x08006000\s*,\s*/\*\s*OTA1[^\n]*\*/\s*"
        r"(0x08106000|0x08286000)\s*,\s*/\*\s*OTA2[^\n]*\*/",
        old_cfg,
    )
    if not cfg_anchor or len(re.findall(r"u32\s+OTA_Region\s*\[\s*2\s*\]", old_cfg)) != 1:
        raise LayoutError("Unrecognized pinned bootcfg OTA_Region")
    if len(re.findall(r"^\s*#define\s+LS_IMG2_OTA1_ADDR\s+0x08006000\b",
                      old_hdr, re.MULTILINE)) != 1:
        raise LayoutError("OTA1 header does not match observed physical backup")
    hdr_match = re.search(
        r"^\s*#define\s+LS_IMG2_OTA2_ADDR\s+(0x08106000|0x08286000)\b",
        old_hdr, re.MULTILINE,
    )
    if not hdr_match or len(re.findall(r"LS_IMG2_OTA2_ADDR", old_hdr)) != 1:
        raise LayoutError("Unrecognized pinned OTA2 header definition")
    if cfg_anchor.group(1) not in (SDK_OTA2, GW006_OTA2) or hdr_match.group(1) not in (SDK_OTA2, GW006_OTA2):
        raise LayoutError("Unknown OTA2 address")
    cfg_start, cfg_end = cfg_anchor.span(1)
    hdr_start, hdr_end = hdr_match.span(1)
    new_cfg = old_cfg[:cfg_start] + GW006_OTA2 + old_cfg[cfg_end:]
    new_hdr = old_hdr[:hdr_start] + GW006_OTA2 + old_hdr[hdr_end:]
    changed = new_cfg != old_cfg or new_hdr != old_hdr
    if apply:
        cfg.write_text(new_cfg)
        hdr.write_text(new_hdr)
    return {
        "status": "BUILD_SOURCE_LAYOUT_ALIGNED",
        "ota1": OTA1,
        "ota2": GW006_OTA2,
        "observed_ota2_dump_offset": "0x286000",
        "changed": bool(apply and changed),
        "device_flash_modified": False,
        "boot_verified_on_GW006": False,
        "installable": False,
    }

def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--sdk", type=Path, required=True)
    p.add_argument("--apply", action="store_true")
    args = p.parse_args()
    try:
        result = configure(args.sdk, args.apply)
    except (OSError, LayoutError) as e:
        print(json.dumps({"status": "BLOCKED", "reason": str(e)}))
        return 2
    print(json.dumps(result, indent=2))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
