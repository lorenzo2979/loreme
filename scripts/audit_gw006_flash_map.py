#!/usr/bin/env python3
"""Offline GW006/WBRG1 reserved-flash partition audit. NEVER reads/writes a chip.

A passing audit is a *necessary*, NOT sufficient, condition for enabling a
physical flash adapter. The real map must come from the target's actual firmware
and boot/update configuration, NOT from guesses or a generic AmebaD+ example.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

SECTOR = 4096
BUTTONS = 96
MAX_FRAMES = 5
MAX_PULSES = 2048
MAX_RECORD = 20 + MAX_FRAMES * (12 + MAX_PULSES * 4)
RECORD_ALIGNED = ((MAX_RECORD + SECTOR - 1) // SECTOR) * SECTOR
MIN_DATA_BYTES = 2 * SECTOR + BUTTONS * RECORD_ALIGNED
# At least one more maximum record must fit, to allow an update without loss.
MIN_WITH_UPDATE = MIN_DATA_BYTES + RECORD_ALIGNED
EXPECTED_CHIP = "WBRG1"
EXPECTED_BOARD = "RSH-GW006"

class AuditError(ValueError):
    pass

def get_int(obj: dict, key: str) -> int:
    v = obj.get(key)
    if type(v) is not int or v < 0 or v > 0xFFFFFFFF:
        raise AuditError(f"{key}: intero non negativo a 32 bit richiesto")
    return v

def span(obj: dict, name: str, total: int) -> tuple[int, int]:
    if not isinstance(obj, dict):
        raise AuditError(f"{name}: intervallo non valido")
    off, size = get_int(obj, "offset"), get_int(obj, "size")
    if not size or off % SECTOR or size % SECTOR or off > total or size > total - off:
        raise AuditError(f"{name}: fuori flash o non allineato a settori da 4096 byte")
    return (off, off + size)

def overlap(a: tuple[int, int], b: tuple[int, int]) -> bool:
    return a[0] < b[1] and b[0] < a[1]

def audit(layout: dict) -> dict:
    if not isinstance(layout, dict):
        raise AuditError("Il documento deve essere un oggetto JSON")
    if layout.get("board") != EXPECTED_BOARD or layout.get("module") != EXPECTED_CHIP:
        raise AuditError("Scheda e modulo non corrispondono al target GW006/WBRG1")
    flash = get_int(layout, "flash_bytes")
    if flash < MIN_WITH_UPDATE or flash % SECTOR:
        raise AuditError("Dimensione fisica flash non valida o insufficiente")
    if layout.get("layout_verified_on_real_board") is not True:
        raise AuditError("Manca conferma esplicita del layout sulla scheda reale")
    digest = layout.get("backup_sha256", "")
    if not isinstance(digest, str) or not re.fullmatch("[0-9a-fA-F]{64}", digest):
        raise AuditError("Serve SHA256 del backup integrale realizzato sulla scheda")
    if not isinstance(layout.get("evidence_note"), str) or len(layout["evidence_note"].strip()) < 20:
        raise AuditError("Serve una nota sulla provenienza e verifica del layout reale")
    reserved = layout.get("reserved")
    if not isinstance(reserved, list) or not reserved:
        raise AuditError("Elenco delle partizioni riservate obbligatorio")
    occupied: list[tuple[str, tuple[int, int]]] = []
    for i, item in enumerate(reserved):
        if not isinstance(item, dict) or not isinstance(item.get("name"), str) or not item["name"].strip():
            raise AuditError(f"reserved[{i}]: nome mancante")
        occupied.append((item["name"], span(item, f"reserved[{i}]", flash)))
    for i, (name, interval) in enumerate(occupied):
        for other, other_interval in occupied[:i]:
            if overlap(interval, other_interval):
                raise AuditError(f"Partizioni riservate sovrapposte: {name} / {other}")
    raw_candidate = layout.get("candidate")
    candidate = span(raw_candidate, "candidate", flash)
    for name, interval in occupied:
        if overlap(candidate, interval):
            raise AuditError(f"Partizione IR sovrapposta a zona protetta: {name}")
    # Require the supplied layout to ACCOUNT FOR EVERY sector. Unknown gaps
    # are an unclassified risk: they might contain OTA, calibration or FTL.
    # This checks the declared map only, NOT its truth on the actual board.
    intervals = sorted([interval for _, interval in occupied] + [candidate])
    frontier = 0
    for start, end in intervals:
        if start != frontier:
            raise AuditError("Mappa incompleta: spazio flash non classificato o partizioni sovrapposte")
        frontier = end
    if frontier != flash:
        raise AuditError("Mappa incompleta: parte finale della flash non classificata")
    capacity = candidate[1] - candidate[0]
    if capacity < MIN_WITH_UPDATE:
        raise AuditError(f"Partizione IR insufficiente: {capacity} < {MIN_WITH_UPDATE} byte")
    # This is not a vendor bootloader verifier and does not authorize writes.
    return {
        "result": "LAYOUT_PLAUSIBLE_NO_WRITE_AUTHORIZATION",
        "flash_bytes": flash,
        "candidate_offset": candidate[0],
        "candidate_size": capacity,
        "min_96_max_records": MIN_DATA_BYTES,
        "min_one_max_update": MIN_WITH_UPDATE,
        "spare_bytes_after_initial_load_and_one_update": capacity - MIN_WITH_UPDATE,
        "reserved_regions": len(occupied),
        "read_only_audit": True,
        "on_device_verified": False,
        "warning": "Il risultato non autorizza scritture fisiche o flashing. Restano da provare bootloader, OTA, driver e recupero."
    }

def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("layout", type=Path, help="JSON con partizioni documentate della vera GW006")
    p.add_argument("--backup", required=True, type=Path,
                   help="Backup fisico integrale della flash della GW006, sola lettura")
    args = p.parse_args()
    try:
        layout = json.loads(args.layout.read_text(encoding="utf-8"))
        total = get_int(layout, "flash_bytes")
        if args.backup.stat().st_size != total:
            raise AuditError("Dimensione backup diverso dalla flash dichiarata")
        h = hashlib.sha256()
        sample_first = None
        all_uniform = True
        with args.backup.open("rb") as f:
            while data := f.read(1024*1024):
                h.update(data)
                if sample_first is None:
                    sample_first = data[0]
                if all_uniform and (data.strip(bytes([sample_first])) != b""):
                    all_uniform = False
        if all_uniform:
            raise AuditError("Backup apparentemente vuoto/uniforme, non verificabile")
        if layout.get("backup_sha256", "").lower() != h.hexdigest():
            raise AuditError("SHA256 del backup diverso da quello dichiarato")
        report = audit(layout)
    except (OSError, json.JSONDecodeError, AuditError) as exc:
        print(json.dumps({"result": "BLOCKED", "reason": str(exc)}, ensure_ascii=False, indent=2))
        return 2
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0

if __name__ == "__main__":
    sys.exit(main())
