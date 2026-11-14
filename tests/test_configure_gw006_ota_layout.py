#!/usr/bin/env python3
"""Host-only tests: never opens a serial port or writes physical flash."""
import importlib.util
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "gw006_ota", ROOT/"scripts/configure_gw006_ota_layout.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

CONFIG = """BOOT_RAM_DATA_SECTION
u32 OTA_Region[2] = {
    0x08006000,        /* OTA1 region start address */
    0x08106000,        /* OTA2 region start address */
};
"""
HEADER = """#define LS_IMG2_OTA1_ADDR 0x08006000
#define LS_IMG2_OTA2_ADDR 0x08106000
"""

class TestGW006OTA(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        (self.root/module.BOOTCFG).parent.mkdir(parents=True)
        (self.root/module.OTAH).parent.mkdir(parents=True)
        (self.root/module.BOOTCFG).write_text(CONFIG)
        (self.root/module.OTAH).write_text(HEADER)
    def test_patch_and_reentry(self):
        a = module.configure(self.root, True)
        self.assertTrue(a["changed"])
        self.assertFalse(a["installable"])
        self.assertIn("0x08286000", (self.root/module.BOOTCFG).read_text())
        self.assertIn("0x08286000", (self.root/module.OTAH).read_text())
        self.assertFalse(module.configure(self.root, True)["changed"])
    def test_dry_run_does_not_write(self):
        module.configure(self.root, False)
        self.assertEqual((self.root/module.BOOTCFG).read_text(), CONFIG)
    def test_unknown_vendor_layout_is_rejected(self):
        (self.root/module.BOOTCFG).write_text(CONFIG.replace("0x08106000", "0x08206000"))
        with self.assertRaises(module.LayoutError):
            module.configure(self.root, True)
    def test_ota_header_mismatch_is_rejected(self):
        (self.root/module.OTAH).write_text(HEADER.replace("0x08106000", "0x08206000"))
        with self.assertRaises(module.LayoutError):
            module.configure(self.root, True)

if __name__ == "__main__":
    unittest.main(verbosity=2)
