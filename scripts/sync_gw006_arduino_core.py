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

ROOT = Path(__file__).resolve().parents[1]
ARCHIVE = ROOT / "source_bundles/GW006_WBRG1_STORAGE_PORT_SOURCE_ONLY.zip"
EXPECTED_SHA256 = "1a97c5f571c85224ea5df923bbf114addb3acea8bf1b65cbf35f3d653c168044"
DEST = ROOT / "firmware/arduino/GW006_Ameba_Probe/src"
COMPONENTS = (
    "gwir_protocol", "gwir_engine", "gwir_gwcd_store",
    "gwir_flash_journal", "gwir_saved_play_command", "gwir_auth",
    "gwir_auth_dispatch", "gwir_auth_stream", "gwir_runtime_owner",
)
PATCHES = (
    "gw006_authenticated_storage_protocol.patch",
    "gw006_authenticated_store_implementation.patch",
    "gw006_authenticated_storage_abort.patch",
)


def sync() -> None:
    if hashlib.sha256(ARCHIVE.read_bytes()).hexdigest() != EXPECTED_SHA256:
        raise SystemExit("GW006 pinned source bundle SHA-256 mismatch")
    with tempfile.TemporaryDirectory(prefix="gw006-arduino-core-") as work:
        work = Path(work)
        with ZipFile(ARCHIVE) as z:
            members = set(z.namelist())
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
        DEST.mkdir(parents=True, exist_ok=True)
        for module in COMPONENTS:
            for suffix in (".c", ".h"):
                source = work / "firmware/core" / (module + suffix)
                (DEST / (module + suffix)).write_bytes(source.read_bytes())
    print("GW006 Arduino: 9 exact patched C modules + 9 headers synchronized.")


if __name__ == "__main__":
    sync()
