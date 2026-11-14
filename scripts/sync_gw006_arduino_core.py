#!/usr/bin/env python3
"""Build the Realtek Arduino sketch with EXACT pinned GWIR IR/storage/auth core.

Extract only allowlisted C files from the hash-pinned source bundle. Apply
the same reviewed authenticated-storage patches as the native Realtek build.
Never write to a device, run an upload tool, or use a public flash map.
"""
import hashlib
import subprocess
import tempfile
from pathlib import Path
from zipfile import ZipFile
from apply_gw006_storage_abort import apply as apply_store_abort

ROOT = Path(__file__).resolve().parents[1]
ARCHIVE = ROOT / "source_bundles/GW006_WBRG1_STORAGE_PORT_SOURCE_ONLY.zip"
EXPECTED_SHA256 = "1a97c5f571c85224ea5df923bbf114addb3acea8bf1b65cbf35f3d653c168044"
DEST = ROOT / "firmware/arduino/GW006_Ameba_Probe/src"
COMPONENTS = (
    "gwir_protocol", "gwir_engine", "gwir_gwcd_store",
    "gwir_flash_journal", "gwir_saved_play_command", "gwir_auth",
    "gwir_auth_dispatch", "gwir_auth_stream", "gwir_runtime_owner",
    "gwir_capture_builder",
)
REALTEK_IR_COMPONENTS = (
    "gwir_amebad_board_pins", "gwir_amebad_bus", "gwir_amebad_rx_fifo",
    "gwir_amebad_rx_irq", "gwir_amebad_rx_vendor",
    "gwir_amebad_tx_codec", "gwir_amebad_tx_fifo",
    "gwir_amebad_tx_vendor", "gwir_amebad_vendor_bus", "gwir_ir_pipeline",
)
APCONFIG_COMPONENTS = (
    "gwir_apconfig_button", "gwir_apconfig_session",
    "gwir_apconfig_controller",
)
PATCHES = (
    "gw006_authenticated_storage_protocol.patch",
    "gw006_authenticated_store_implementation.patch",
)


def sync() -> None:
    if hashlib.sha256(ARCHIVE.read_bytes()).hexdigest() != EXPECTED_SHA256:
        raise SystemExit("GW006 pinned source bundle SHA-256 mismatch")
    with tempfile.TemporaryDirectory(prefix="gw006-arduino-core-") as work:
        work = Path(work)
        with ZipFile(ARCHIVE) as z:
            members = set(z.namelist())
            flash_sources = {}
            realtek_ir_sources = {}
            for component in REALTEK_IR_COMPONENTS:
                for suffix in (".c", ".h"):
                    name = "firmware/realtek/" + component + suffix
                    if name not in members:
                        raise SystemExit("Missing pinned IR source: " + name)
                    realtek_ir_sources[component + suffix] = z.read(name).decode("utf-8")
            for suffix in (".c", ".h"):
                name = "firmware/realtek/gwir_wbrg1_flash_port" + suffix
                if name not in members:
                    raise SystemExit("Pinned adapter missing: " + name)
                flash_sources[suffix] = z.read(name).decode("utf-8")
            for module in COMPONENTS:
                for suffix in (".c", ".h"):
                    name = "firmware/core/" + module + suffix
                    if name not in members:
                        raise SystemExit("Missing pinned source " + name)
                    dst = work / name
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    dst.write_bytes(z.read(name))
        for patch_name in PATCHES:
            patch = ROOT / "patches" / patch_name
            for check in (True, False):
                args = ["git", "apply"]
                if check:
                    args.append("--check")
                args.append(str(patch))
                result = subprocess.run(args, cwd=work, capture_output=True, text=True)
                if result.returncode:
                    raise SystemExit(f"Patch {patch_name} check={check} failed: "
                                     f"{result.stdout} {result.stderr}")
        apply_store_abort(work / "firmware/core")
        DEST.mkdir(parents=True, exist_ok=True)
        # The official Realtek NOR adapter is part of the same pinned source.
        # Copy it into the Arduino build with only its include path adapted.
        # Runtime writes remain disabled unless an explicitly reviewed
        # WBRG1-only partition is authorized (default=0).
        for suffix in (".c", ".h"):
            path = "firmware/realtek/gwir_wbrg1_flash_port" + suffix
            source = flash_sources[suffix]
            source = source.replace('#include "../core/gwir_flash_journal.h"',
                                    '#include "gwir_flash_journal.h"')
            (DEST / ("gwir_wbrg1_flash_port" + suffix)).write_text(source)
        # The branch carries the newer, reviewed PA25/PA26 hardware gate.
        # The archived WBRG1 source bundle predates that board-setup symbol.
        # Use the tracked native adapter so the Arduino image links to the
        # exact GPIO implementation documented by the current project.
        updated_bus = (ROOT / "firmware/realtek/gwir_amebad_vendor_bus.c").read_text()
        if "int gwir_amebad_vendor_board_ir_setup(void)" not in updated_bus:
            raise SystemExit("Current PA25/PA26 board setup implementation missing")
        realtek_ir_sources["gwir_amebad_vendor_bus.c"] = updated_bus
        for name, source in realtek_ir_sources.items():
            source = source.replace('#include "../core/gwir_protocol.h"',
                                    '#include "gwir_protocol.h"')
            source = source.replace('#include "../core/gwir_capture_builder.h"',
                                    '#include "gwir_capture_builder.h"')
            (DEST / name).write_text(source)
        for module in COMPONENTS:
            for suffix in (".c", ".h"):
                source = work / "firmware/core" / (module + suffix)
                (DEST / (module + suffix)).write_bytes(source.read_bytes())
    # Wire the host-reviewed PB23 button -> bounded APConfig session
    # controller into the native image. All board approval gates remain OFF.
    for component in APCONFIG_COMPONENTS:
        for suffix in (".c", ".h"):
            src = ROOT / "firmware/core" / (component + suffix)
            if not src.is_file():
                raise SystemExit("Missing tracked APConfig controller source: " + str(src))
            (DEST / src.name).write_bytes(src.read_bytes())
    print("GW006 Arduino: 10 core + 10 Realtek IR + NOR + 3 APConfig controller modules synced.")


if __name__ == "__main__":
    sync()
