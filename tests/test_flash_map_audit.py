#!/usr/bin/env python3
"""Host-only validation; synthetic maps are NOT a mapping of the physical GW006."""
import json
import hashlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from audit_gw006_flash_map import audit, AuditError, MIN_WITH_UPDATE, SECTOR

class TestMap(unittest.TestCase):
    def setUp(self):
        self.valid = {
            "board": "RSH-GW006", "module": "WBRG1", "flash_bytes": 8 * 1024 * 1024,
            "layout_verified_on_real_board": True,
            "backup_sha256": "0" * 64,
            "evidence_note": "SOLO TEST SINTETICO: non usare sulla scheda reale",
            "reserved": [
                {"name": "synthetic-boot-firmware-ota", "offset": 0, "size": 3 * 1024 * 1024},
                {"name": "synthetic-end", "offset": 0x780000, "size": 0x80000}
            ],
            "candidate": {"offset": 3 * 1024 * 1024, "size": 0x480000}
        }
    def test_good_synthetic(self):
        r = audit(self.valid)
        self.assertEqual(r["result"], "LAYOUT_PLAUSIBLE_NO_WRITE_AUTHORIZATION")
        self.assertGreaterEqual(r["candidate_size"], MIN_WITH_UPDATE)
        self.assertTrue(r["read_only_audit"])
        self.assertFalse(r["on_device_verified"])
    def blocked(self, adjust):
        d = json.loads(json.dumps(self.valid)); adjust(d)
        with self.assertRaises(AuditError): audit(d)
    def test_missing_proof(self):
        self.blocked(lambda d: d.update(layout_verified_on_real_board=False))
        self.blocked(lambda d: d.update(backup_sha256="missing"))
        self.blocked(lambda d: d.update(evidence_note=""))
    def test_wrong_board_and_layout(self):
        self.blocked(lambda d: d.update(module="ESP32"))
        self.blocked(lambda d: d.update(board="GW007"))
        self.blocked(lambda d: d.update(reserved=[]))
        self.blocked(lambda d: d.update(reserved="none"))
    def test_overlap_boot_ota(self):
        self.blocked(lambda d: d["candidate"].update(offset=0))
        self.blocked(lambda d: d["candidate"].update(offset=0x77F000))
        self.blocked(lambda d: d["reserved"].append({"name": "new", "offset":0x1000,"size":0x1000}))
    def test_no_unclassified_gaps(self):
        self.blocked(lambda d: d["reserved"][0].update(size=2*1024*1024))
        self.blocked(lambda d: d["reserved"][1].update(offset=0x790000,size=0x70000))
    def test_boundary_and_overflow(self):
        self.blocked(lambda d: d["candidate"].update(offset=0xFFFFFFFF))
        self.blocked(lambda d: d["candidate"].update(size=0xFFFFFFFF))
        self.blocked(lambda d: d["candidate"].update(offset=0x300001))
        self.blocked(lambda d: d["candidate"].update(size=SECTOR))
        self.blocked(lambda d: d.update(flash_bytes=4*1024*1024))
        self.blocked(lambda d: d.update(flash_bytes=True))
        self.blocked(lambda d: d["reserved"][0].update(size=-1))
    def test_cli_default_blocks(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/"layout.json"
            d=json.loads(json.dumps(self.valid))
            d["layout_verified_on_real_board"]=False
            p.write_text(json.dumps(d))
            backup=Path(tmp)/"chip.bin"
            backup.write_bytes(bytes(range(256))*32768)  # synthetic 8 MiB; NOT physical GW006
            d["backup_sha256"] = hashlib.sha256(backup.read_bytes()).hexdigest()
            p.write_text(json.dumps(d))
            argv=[sys.executable,str(ROOT/"scripts/audit_gw006_flash_map.py"),str(p),"--backup",str(backup)]
            r=subprocess.run(argv,capture_output=True,text=True)
            self.assertEqual(r.returncode,2)
            self.assertEqual(json.loads(r.stdout)["result"], "BLOCKED")
            d["layout_verified_on_real_board"] = True
            p.write_text(json.dumps(d))
            r=subprocess.run(argv,capture_output=True,text=True)
            self.assertEqual(r.returncode,0)
            self.assertEqual(json.loads(r.stdout)["result"], "LAYOUT_PLAUSIBLE_NO_WRITE_AUTHORIZATION")
            d["backup_sha256"] = "0"*64
            p.write_text(json.dumps(d))
            r=subprocess.run(argv,capture_output=True,text=True)
            self.assertEqual(r.returncode,2)
            self.assertIn("SHA256",r.stdout)

if __name__ == "__main__":
    unittest.main(verbosity=2)
