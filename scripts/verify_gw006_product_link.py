#!/usr/bin/env python3
"""GW006 native product link: require all LIVE modules and exclude retired code.

Native vendor ELF/map + full 46-object source attestation remain mandatory.
This explicit product profile is stricter than merely making all 46 object
names appear in the binary: three historic plaintext/GET-only interfaces
MUST be discarded, not activated to satisfy a generic linker score.
No hardware, UART, flash write or deployment is attempted.
"""
from __future__ import annotations
import argparse
import json
import sys
from pathlib import Path

from verify_native_km4_objects import inspect as verify_objects
from verify_native_km4_link_map import (
    IMAGE_DIR, MAP_NAME, ELF_NAME, analyze_map_report, safe_build_artifact,
    verify_arm_linked_elf,
)
from verify_release_sources import safe_path, verify_compiled_source_inventory

ROOT = Path(__file__).resolve().parents[1]
SOURCE_COUNT = 46
# Superseded and deliberately *absent* from final SRAM/ROM sections:
#   legacy GWIR plaintext stream -> now authenticated GWAS stream;
#   standalone GET dashboard -> now signed RPC through single owner;
#   unused STA GET-only launcher -> never commissioned on product.
RETIRED = frozenset({
    "gwir_stream", "gwir_dashboard_sta_server", "gwir_sdk_sta_dashboard",
})


def classify(report: dict, modules: set[str]) -> dict:
    if len(modules) != SOURCE_COUNT or not RETIRED.issubset(modules):
        raise ValueError("Expected exact reviewed 46-module product inventory")
    live = modules - RETIRED
    present = set(report["allocated_modules"])
    loaded = set(modules) - set(report["not_loaded"])
    leaked = sorted(RETIRED & present)
    missing_load = sorted(live - loaded)
    missing_allocation = sorted(live - present)
    if leaked:
        raise ValueError("Retired unauthenticated components linked: " + repr(leaked))
    if missing_load or missing_allocation:
        raise ValueError(
            "Live GW006 KM4 modules missing: not_loaded=" + repr(missing_load)
            + "; no_allocated_sections=" + repr(missing_allocation))
    return {
        "source_modules_compiled_and_attested": SOURCE_COUNT,
        "live_product_modules_allocated": len(live),
        "retired_modules_excluded_from_final_link": sorted(RETIRED),
        "all_live_modules_linked": True,
        "retired_plaintext_services_absent": True,
        "native_product_link_gate_pass": True,
        "physical_board_test_pass": False,
        "ir_tx_rx_verified": False,
        "flash_layout_approved": False,
        "firmware_installable": False,
    }


def inspect(sdk: Path, start: Path, output: Path | None = None) -> dict:
    sdk = sdk.resolve(strict=True)
    objects = verify_objects(sdk, "postbuild", start)
    manifest = json.loads(
        safe_path(ROOT, "docs/CP21_SOURCE_MANIFEST.json").read_text(encoding="utf-8")
    )
    count = verify_compiled_source_inventory(ROOT, manifest)
    if count != SOURCE_COUNT or objects["native_objects_verified"] != SOURCE_COUNT:
        raise ValueError("Not exactly 46 original source objects verified")
    modules = {
        Path(item["path"]).stem
        for item in manifest["files"]
        if item["path"].startswith("firmware/") and item["path"].endswith(".c")
    }
    if len(modules) != count:
        raise ValueError("Duplicate firmware C module stems")
    start_ns = start.stat().st_mtime_ns
    map_bytes = safe_build_artifact(sdk / IMAGE_DIR / MAP_NAME, 100, start_ns)
    elf_bytes = safe_build_artifact(sdk / IMAGE_DIR / ELF_NAME, 52, start_ns)
    verify_arm_linked_elf(elf_bytes)
    report = analyze_map_report(map_bytes.decode("utf-8"), modules)
    product = classify(report, modules)
    product["native_elf_validated"] = True
    product["source_verified"] = True
    product["linked_evidence"] = True
    product["native_sdk_flash_performed"] = False
    product["description"] = "Realtek ARM native software link, NOT tested on GW006"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(product, indent=2, sort_keys=True) + "\n")
    return product


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--build-start-file", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        result = inspect(args.sdk, args.build_start_file, args.output)
    except (OSError, ValueError, UnicodeError, KeyError) as e:
        print("GW006_LIVE_PRODUCT_LINK=FAIL: " + str(e), file=sys.stderr)
        return 2
    print("GW006_LIVE_PRODUCT_LINK=PASS; NON INSTALLARE; " + json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
