#!/usr/bin/env python3
"""GW006 WBRG1: strictly read-only comparison of TWO full device flash dumps.

No serial interface, network, flash commands or writes to a device.
No physical partition can be approved by this script. The matching images
must have been read INDEPENDENTLY from the user's actual device, not copied.
Keep original dumps private; JSON omits flash contents, keys, SSID and names.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

FLASH_SIZE = 8 * 1024 * 1024
SECTOR = 4096
CHUNK = 1024 * 1024
# This range was a historical proposal, NEVER a safe/verified partition.
UNAPPROVED_CANDIDATE = (0x200000, 0x700000)
NO_RELEASE = "BLOCKED_NO_DEVICE_LAYOUT_OR_RECOVERY"

class BackupError(ValueError):
    pass


def compare_dumps(first: Path, second: Path) -> dict:
    if first.resolve() == second.resolve():
        raise BackupError("Serve una seconda lettura indipendente; stesso file vietato")
    if not first.is_file() or not second.is_file():
        raise BackupError("Entrambe le copie devono esistere come file")
    a = first.stat().st_size
    b = second.stat().st_size
    if a != FLASH_SIZE or b != FLASH_SIZE:
        raise BackupError(
            "Backup incompleto: servono 8.388.608 byte per copia; "
            f"dimensioni rilevate {a} e {b}"
        )
    h = hashlib.sha256()
    all_ff = 0
    all_zero = 0
    non_ff_candidate = 0
    first_mib = None
    longest_ff = 0
    current_ff = 0
    prev = 0
    sectors = 0
    # Iterate in 4096-byte sectors, without loading entire private flash dump
    # into memory. Reads from both backups; never opens either for writing.
    with first.open("rb") as fa, second.open("rb") as fb:
        for offset in range(0, FLASH_SIZE, SECTOR):
            ba = fa.read(SECTOR)
            bb = fb.read(SECTOR)
            if len(ba) != SECTOR or len(bb) != SECTOR:
                raise BackupError("Lettura incompleta durante il confronto")
            if ba != bb:
                ix = next(i for i, (x, y) in enumerate(zip(ba, bb)) if x != y)
                raise BackupError(
                    f"Due letture diverse al byte 0x{offset + ix:06X}: "
                    "risolvere il problema prima di qualsiasi operazione"
                )
            h.update(ba)
            sectors += 1
            if all(x == 0xFF for x in ba):
                all_ff += 1
                current_ff += 1
                if current_ff > longest_ff:
                    longest_ff = current_ff
            else:
                current_ff = 0
            if all(x == 0x00 for x in ba):
                all_zero += 1
            if UNAPPROVED_CANDIDATE[0] <= offset < UNAPPROVED_CANDIDATE[1]:
                if any(x != 0xFF for x in ba):
                    non_ff_candidate += 1
            # Reject wholly uniform 8 MiB, not just all-FF.
            for x in ba:
                if first_mib is None:
                    first_mib = x
                elif x != first_mib:
                    prev = 1
    if not prev:
        raise BackupError("Dump uniformemente ripetuto (0xFF, 0x00 o altro): lettura sospetta")
    return {
        "result": "TWO_FILES_MATCH_READ_ONLY",
        "full_flash_bytes": FLASH_SIZE,
        "read_a_bytes": a,
        "read_b_bytes": b,
        "sha256_both": h.hexdigest(),
        "match_exactly": True,
        "sectors_examined": sectors,
        "sector_bytes": SECTOR,
        "all_ff_sectors": all_ff,
        "all_zero_sectors": all_zero,
        "longest_contiguous_ff_bytes": longest_ff * SECTOR,
        "unapproved_0x200000_0x700000_non_ff_sectors": non_ff_candidate,
        "physical_independent_reads_proven": False,
        "real_partition_layout_verified": False,
        "bootloader_and_recovery_verified": False,
        "gw006_storage_region_approved": False,
        "flash_allowed": False,
        "release_gate": NO_RELEASE,
        "warning": (
            "La coincidenza di due file NON prova che siano due letture "
            "indipendenti, ne' certifica la mappa partizioni. Anche i settori "
            "0xFF possono essere firmware OTA o dati riservati. "
            "Nessun comando di programmazione e' autorizzato."
        ),
    }


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("copia_1", type=Path, help="prima lettura completa, PRIVATA")
    p.add_argument("copia_2", type=Path, help="seconda lettura indipendente, PRIVATA")
    p.add_argument("--json", type=Path, default=None,
                   help="salva solo il rapporto privo di dati della flash")
    args = p.parse_args(argv)
    try:
        report = compare_dumps(args.copia_1, args.copia_2)
    except (OSError, BackupError) as exc:
        report = {
            "result": "BLOCKED",
            "reason": str(exc),
            "flash_allowed": False,
            "release_gate": NO_RELEASE,
        }
        print(json.dumps(report, ensure_ascii=False, indent=2))
        return 2
    if args.json:
        if args.json.resolve() in (args.copia_1.resolve(), args.copia_2.resolve()):
            print('Impossibile sovrascrivere una copia del backup', file=sys.stderr)
            return 2
        try:
            args.json.write_text(
                json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                encoding="utf-8")
        except OSError as exc:
            print(f"Impossibile salvare rapporto: {exc}", file=sys.stderr)
            return 2
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
