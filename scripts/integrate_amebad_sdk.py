#!/usr/bin/env python3
"""Non-flashing, idempotent KM4 source integration into the vendor AmebaD SDK.

Only changes SDK source files when --apply is supplied. Does NOT alter the
bootloader, pinmux, ROM, security flags, firmware images, or the GW006 board.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import shutil
import sys
from pathlib import Path

SCRIPT_ROOT = Path(__file__).resolve().parents[1]
SDK_PROJECT = Path("project/realtek_amebaD_va0_example")
TARGET_RELPATH = SDK_PROJECT / "src" / "gwir_sdk"
MAIN_RELPATH = SDK_PROJECT / "src" / "src_hp" / "main.c"
MAKE_RELPATH = SDK_PROJECT / "src" / "src_hp" / "Makefile"
BEGIN_MK = "# >>> GWIR_REV9_START (owned by integrator)"
END_MK = "# <<< GWIR_REV9_END"
BEGIN_C = "/* >>> GWIR_REV9_START (owned by integrator) */"
END_C = "/* <<< GWIR_REV9_END */"

GROUPS = {
    "core": ["gwir_protocol.c", "gwir_engine.c", "gwir_capture_builder.c", "gwir_stream.c"],
    "realtek": [
        "gwir_amebad_tx_codec.c", "gwir_amebad_tx_fifo.c", "gwir_amebad_tx_vendor.c",
        "gwir_amebad_rx_fifo.c", "gwir_amebad_rx_vendor.c", "gwir_amebad_rx_irq.c",
    ],
    "sdk": ["gwir_sdk_diag.c"],
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def require_sdk(root: Path) -> None:
    paths = [MAIN_RELPATH, MAKE_RELPATH, SDK_PROJECT / "GCC-RELEASE/project_hp/asdk/Makefile.include.gen",
             Path("component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h"),
             Path("component/soc/realtek/amebad/fwlib/ram_common/rtl8721d_ir.c")]
    missing = [str(path) for path in paths if not (root / path).is_file()]
    if missing:
        raise ValueError("AmebaD SDK incomplete/incorrect; required files missing:\n- " + "\n- ".join(missing))


def safe_relative(p: Path) -> None:
    if p.is_symlink():
        raise ValueError(f"Symlink prohibited as integration destination: {p}")


def block_replace(text: str, begin: str, end: str, insert_after: str, block: str) -> str:
    if begin in text or end in text:
        if text.count(begin) != 1 or text.count(end) != 1:
            raise ValueError(f"Inconsistent existing {begin} block")
        a, b = text.index(begin), text.index(end) + len(end)
        if a >= b:
            raise ValueError("Invalid block bounds")
        # Preserve any pre-existing identical integration; otherwise replace
        # the owned region only, leaving vendor text untouched.
        return text[:a] + begin + "\n" + block.rstrip() + "\n" + end + text[b:]
    if text.count(insert_after) != 1:
        raise ValueError(f"Vendor anchor missing or duplicated: {insert_after!r}")
    return text.replace(insert_after, insert_after + "\n" + begin + "\n" + block.rstrip() + "\n" + end, 1)


def make_contents(old: str) -> str:
    # Source paths are absolute via SRCDIR, so no SDK toolchain flags overridden.
    body = ["# Core + Realtek HAL helpers compiled into the HP image, NO hardware activation.",
            "GWIR_DIR := $(DIR)/../gwir_sdk",
            "MODULE_IFLAGS += -I$(GWIR_DIR)/core -I$(GWIR_DIR)/realtek -I$(GWIR_DIR)/sdk",
            "vpath %.c $(GWIR_DIR)/core $(GWIR_DIR)/realtek $(GWIR_DIR)/sdk"]
    for group, names in GROUPS.items():
        body.append("CSRC += " + " ".join("$(GWIR_DIR)/%s/%s" % (group, name) for name in names))
    return block_replace(old, BEGIN_MK, END_MK, "CSRC = $(DIR)/main.c", "\n".join(body))


def main_contents(old: str) -> str:
    # Call just before the scheduler starts; logging only, no pins initialized.
    if BEGIN_C in old or END_C in old:
        return block_replace(old, BEGIN_C, END_C, "\tvTaskStartScheduler();",
           "\textern void gwir_sdk_integration_banner(void);\n\tgwir_sdk_integration_banner();")
    anchor = "\tvTaskStartScheduler();"
    if old.count(anchor) != 1:
        raise ValueError("SDK main.c scheduler anchor missing or repeated")
    block = BEGIN_C + "\n\textern void gwir_sdk_integration_banner(void);\n\tgwir_sdk_integration_banner();\n" + END_C + "\n"
    return old.replace(anchor, block + anchor, 1)


def strip_owned_block(s: str, begin: str, end: str) -> str:
    if begin not in s and end not in s:
        return s
    if s.count(begin) != 1 or s.count(end) != 1:
        raise ValueError("Cannot safely remove edited/mismatched owned block")
    a, b = s.index(begin), s.index(end) + len(end)
    if a >= b:
        raise ValueError("Invalid owned block")
    # Remove only generated block and its adjacent linebreak.
    if b < len(s) and s[b] == "\n":
        b += 1
    return s[:a] + s[b:]


def file_plan():
    plan = {}
    for group, names in GROUPS.items():
        for file in names:
            src = SCRIPT_ROOT / "firmware" / group / file
            if not src.is_file():
                raise ValueError(f"Missing REV9 source {src}")
            plan[TARGET_RELPATH / group / file] = src.read_bytes()
    # Header files have relative inclusion paths; copy matching headers too.
    for group in ("core", "realtek"):
        for src in sorted((SCRIPT_ROOT / "firmware" / group).glob("*.h")):
            plan[TARGET_RELPATH / group / src.name] = src.read_bytes()
    return plan


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--sdk", required=True, type=Path, help="Path to COMPLETE Ameba-AIoT/ameba-rtos-d checkout")
    opts = p.add_mutually_exclusive_group(required=True)
    opts.add_argument("--apply", action="store_true", help="Copy sources and patch two vendor project files")
    opts.add_argument("--dry-run", action="store_true", help="Validate without writing anything")
    opts.add_argument("--remove", action="store_true", help="Remove own blocks/files without touching vendor originals")
    args = p.parse_args()
    root = args.sdk.resolve(strict=True)
    if not root.is_dir() or str(root) in {"/", "/mnt/data"}:
        raise ValueError("Invalid SDK directory")
    require_sdk(root)
    mf, cf = root / MAKE_RELPATH, root / MAIN_RELPATH
    for f in (mf, cf, root / TARGET_RELPATH):
        safe_relative(f)
    original_m, original_c = mf.read_text(), cf.read_text()
    if args.remove:
        new_m = strip_owned_block(original_m, BEGIN_MK, END_MK)
        new_c = strip_owned_block(original_c, BEGIN_C, END_C)
        # Never delete a user-modified file; verify it still matches REV9.
        plan = file_plan()
        for rel, expected in plan.items():
            f = root / rel
            if f.is_file() and sha(f.read_bytes()) != sha(expected):
                raise ValueError(f"Refusing removal: modified integrated file: {rel}")
        if new_m != original_m: mf.write_text(new_m)
        if new_c != original_c: cf.write_text(new_c)
        for rel in plan:
            f = root / rel
            if f.is_file(): f.unlink()
        for group in ("core", "realtek", "sdk"):
            d = root / TARGET_RELPATH / group
            if d.exists() and not list(d.iterdir()): d.rmdir()
        d = root / TARGET_RELPATH
        if d.exists() and not list(d.iterdir()): d.rmdir()
        print("REV9 integration removed (only owned text and matching sources)")
        return 0
    new_m, new_c = make_contents(original_m), main_contents(original_c)
    plan = file_plan()
    collisions = []
    for rel, data in plan.items():
        dst = root / rel
        safe_relative(dst)
        if dst.exists() and sha(dst.read_bytes()) != sha(data):
            collisions.append(str(rel))
    if collisions:
        raise ValueError("Refusing overwrite of different files:\n- " + "\n- ".join(collisions))
    report = {"sdk_root": str(root), "mode": "apply" if args.apply else "dry-run",
              "makefile_changed": new_m != original_m, "main_changed": new_c != original_c,
              "files": len(plan), "sources": sum(len(v) for v in GROUPS.values()),
              "vendor_pinmux_modified": False, "board_flashing": False}
    if args.apply:
        # Keep explicit non-destructive backups for recovery.
        for f, old in ((mf, original_m), (cf, original_c)):
            bak = f.with_name(f.name + ".gw006-before-rev9")
            if not bak.exists(): bak.write_text(old)
        for rel, data in plan.items():
            dst = root / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(data)
        mf.write_text(new_m)
        cf.write_text(new_c)
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as e:
        print("SDK integration error:", e, file=sys.stderr)
        raise SystemExit(2)
