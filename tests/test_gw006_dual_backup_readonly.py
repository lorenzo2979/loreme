#!/usr/bin/env python3
"""Synthetic host tests only. Never access a GW006, USB/UART or a real dump."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from verify_gw006_dual_backup_readonly import (
    FLASH_SIZE, BackupError, compare_dumps,
)


def synthetic(path, length=FLASH_SIZE):
    # NOT a real firmware image: mostly FF, a handful of non-FF test bytes.
    with path.open("wb") as fd:
        fd.write(b"\xff" * length)
    if length == FLASH_SIZE:
        with path.open("r+b") as fd:
            for off, value in [(0, 0x52), (0x200000, 0x47),
                               (0x500000, 0x03), (FLASH_SIZE-1, 0x80)]:
                fd.seek(off)
                fd.write(bytes([value]))


class DualCopy(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.a = Path(self.temp.name) / "chip_a_private.bin"
        self.b = Path(self.temp.name) / "chip_b_private.bin"
        synthetic(self.a)
        self.b.write_bytes(self.a.read_bytes())

    def test_two_matching_synthetic_files_never_authorize_flash(self):
        report = compare_dumps(self.a, self.b)
        self.assertEqual(report["result"], "TWO_FILES_MATCH_READ_ONLY")
        self.assertEqual(report["full_flash_bytes"], 8388608)
        self.assertEqual(report["sectors_examined"], 2048)
        self.assertEqual(report["unapproved_0x200000_0x700000_non_ff_sectors"], 2)
        self.assertEqual(report["sha256_both"],
                         hashlib.sha256(self.a.read_bytes()).hexdigest())
        self.assertFalse(report["flash_allowed"])
        self.assertFalse(report["real_partition_layout_verified"])
        self.assertFalse(report["physical_independent_reads_proven"])

    def test_byte_mismatch_blocks(self):
        with self.b.open("r+b") as fd:
            fd.seek(0x234567)
            fd.write(b"X")
        with self.assertRaisesRegex(BackupError, "0x234567"):
            compare_dumps(self.a, self.b)

    def test_short_file_blocks(self):
        with self.b.open("r+b") as fd:
            fd.truncate(FLASH_SIZE-1)
        with self.assertRaisesRegex(BackupError, "8.388.608"):
            compare_dumps(self.a, self.b)

    def test_uniform_fake_data_blocks(self):
        self.a.write_bytes(b"\xff" * FLASH_SIZE)
        self.b.write_bytes(b"\xff" * FLASH_SIZE)
        with self.assertRaisesRegex(BackupError, "uniform"):
            compare_dumps(self.a, self.b)

    def test_same_source_path_blocks(self):
        with self.assertRaisesRegex(BackupError, "indipendente"):
            compare_dumps(self.a, self.a)

    def test_cli_report_has_no_original_content_or_device_auth(self):
        report = Path(self.temp.name) / "report.json"
        r = subprocess.run([
            sys.executable, str(ROOT/"scripts/verify_gw006_dual_backup_readonly.py"),
            str(self.a), str(self.b), "--json", str(report),
        ], capture_output=True, text=True, check=False)
        self.assertEqual(r.returncode, 0, r.stderr)
        obj = json.loads(report.read_text())
        self.assertFalse(obj["flash_allowed"])
        self.assertEqual(obj["result"], "TWO_FILES_MATCH_READ_ONLY")
        self.assertNotIn(self.a.name, report.read_text())
        self.assertNotIn(self.b.name, report.read_text())
        self.assertNotIn("PASSWORD", report.read_text())

    def test_cli_never_overwrites_original_binary(self):
        original = hashlib.sha256(self.a.read_bytes()).hexdigest()
        r = subprocess.run([
            sys.executable, str(ROOT/"scripts/verify_gw006_dual_backup_readonly.py"),
            str(self.a), str(self.b), "--json", str(self.a),
        ], capture_output=True, text=True, check=False)
        self.assertEqual(r.returncode, 2)
        self.assertEqual(hashlib.sha256(self.a.read_bytes()).hexdigest(),
                         original)


if __name__ == "__main__":
    unittest.main(verbosity=2)
