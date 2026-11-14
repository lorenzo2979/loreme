#!/usr/bin/env python3
"""Positive, invalid and secret-redaction cases for local OBK template reader."""
import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from import_openbeken_template import parse_template


class TestOBKTemplate(unittest.TestCase):
    def test_gw006_pin_aliases(self):
        src = {"chip": "RTL8720D", "pins": {
            "25": "LED_n;0", "26": "IRRecv;0",
            "54": "WiFi_LED;0", "55": "Btn;0", "63": "IRSend;0"}}
        d = parse_template(json.dumps(src))
        self.assertEqual([e["realtek_gpio"] for e in d["gpio_assignments"]],
                         ["PA25", "PA26", "PB22", "PB23", "PB31"])
        self.assertTrue(all(not e["physical_signal_verified"] for e in d["gpio_assignments"]))
        self.assertTrue(d["no_flash_write_permission"])

    def test_secrets_never_in_report(self):
        raw = json.dumps({"chip":"RTL8720D", "pins":{"26":"IRRecv;0"},
                          "ssid":"secret_ssid_123", "password":"secret_pwd_123",
                          "command":"password secret_pwd_123",
                          "device_id":"private_serial"})
        output = json.dumps(parse_template(raw))
        for secret in ("secret_ssid_123", "secret_pwd_123", "private_serial", "command"):
            self.assertNotIn(secret, output)

    def test_reject_bad_json_and_out_of_range(self):
        for raw in ('{"pins":{"63":"LED;0","63":"Btn;0"}}',
                    '{"pins":{"64":"LED;0"}}',
                    '{"pins":{"00":"LED;0"}}',
                    '{"pins":{"25":"LED;0;extra"}}',
                    '{"pins":{"25":"LED;bad"}}',
                    '{"pins":{"25":"LED;256"}}',
                    '{"pins":[]}'):
            with self.subTest(raw=raw):
                with self.assertRaises(ValueError):
                    parse_template(raw)

    def test_empty_pins_is_safe(self):
        result=parse_template('{"pins":{}}')
        self.assertEqual(result["gpio_assignments"], [])
        self.assertFalse(result["does_not_certify_safe_firmware_installation"] is False)

if __name__ == "__main__":
    unittest.main()
