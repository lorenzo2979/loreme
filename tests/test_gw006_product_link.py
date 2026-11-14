#!/usr/bin/env python3
"""Synthetic negative tests for the product linker policy; never hardware I/O."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from verify_gw006_product_link import SOURCE_COUNT, RETIRED, classify

MODULES = set(RETIRED) | {"gwir_required_%02d" % i for i in range(SOURCE_COUNT - len(RETIRED))}
LIVE = MODULES - RETIRED


class ProductLinkPolicy(unittest.TestCase):
    def test_product_requires_all_live_and_excludes_retired(self):
        data = {"allocated_modules": sorted(LIVE), "not_loaded": []}
        got = classify(data, MODULES)
        self.assertEqual(got["live_product_modules_allocated"], 43)
        self.assertTrue(got["retired_plaintext_services_absent"])
        self.assertFalse(got["firmware_installable"])

    def test_missing_live_link_rejects(self):
        data = {"allocated_modules": sorted(LIVE - {"gwir_required_00"}), "not_loaded": []}
        with self.assertRaisesRegex(ValueError, "Live GW006 KM4 modules missing"):
            classify(data, MODULES)

    def test_retired_plaintext_link_rejects(self):
        data = {"allocated_modules": sorted(LIVE | {"gwir_stream"}), "not_loaded": []}
        with self.assertRaisesRegex(ValueError, "Retired unauthenticated"):
            classify(data, MODULES)

    def test_no_load_for_required_rejects(self):
        data = {"allocated_modules": sorted(LIVE), "not_loaded": ["gwir_required_01"]}
        with self.assertRaisesRegex(ValueError, "Live GW006 KM4 modules missing"):
            classify(data, MODULES)

    def test_missing_manifest_module_rejects(self):
        data = {"allocated_modules": sorted(LIVE), "not_loaded": []}
        with self.assertRaisesRegex(ValueError, "exact reviewed 46"):
            classify(data, MODULES - {"gwir_required_00"})


if __name__ == "__main__":
    unittest.main(verbosity=2)
