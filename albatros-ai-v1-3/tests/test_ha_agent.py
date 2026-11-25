"""Regression tests for Gemini's read-only Home Assistant tool integration."""
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from agent import ask, connect, tool_execute


class HAToolAgentTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.db = connect(Path(self.temp.name) / "test.sqlite3")

    def tearDown(self):
        self.db.close()
        self.temp.cleanup()

    def test_default_mock_readings(self):
        with patch.dict(os.environ, {}, clear=True):
            result = tool_execute(self.db, "ha_get_state",
                                  {"entity_id": "sensor.albatros_demo_temperatura"})
            self.assertEqual(result["source"], "mock")
            self.assertEqual(result["data"]["state"], "22.5")
            all_states = tool_execute(self.db, "ha_list_states", {})
            self.assertEqual(all_states["source"], "mock")
            self.assertEqual(len(all_states["data"]), 2)

    def test_no_ha_commands(self):
        with self.assertRaises(ValueError):
            tool_execute(self.db, "ha_call_service",
                         {"domain": "light", "service": "turn_on"})

    def test_invalid_args(self):
        with self.assertRaises(ValueError):
            tool_execute(self.db, "ha_get_state", {"entity_id": "light.test", "extra": 1})
        with self.assertRaises(ValueError):
            tool_execute(self.db, "ha_list_states", {"unused": True})

    def test_live_requires_allowlist(self):
        with patch.dict(os.environ, {"ALBATROS_HA_MODE": "live",
                                     "ALBATROS_HA_URL": "http://localhost:8123",
                                     "ALBATROS_HA_TOKEN": "fake"}, clear=True):
            with self.assertRaises(ValueError):
                tool_execute(self.db, "ha_list_states", {})

    def test_live_deny_before_network(self):
        with patch.dict(os.environ, {"ALBATROS_HA_MODE": "live",
                                     "ALBATROS_HA_URL": "http://localhost:8123",
                                     "ALBATROS_HA_TOKEN": "fake",
                                     "ALBATROS_HA_ENTITIES": "sensor.allowed"}, clear=True):
            with patch("ha_readonly.HAReadOnly._fetch",
                       side_effect=AssertionError("Network access forbidden")):
                with self.assertRaises(ValueError):
                    tool_execute(self.db, "ha_get_state",
                                 {"entity_id": "sensor.not_allowed"})

    def test_live_only_allowlisted_states(self):
        values = {"sensor.allowed": {"entity_id": "sensor.allowed", "state": "ok",
                                     "attributes": {"friendly_name": "Demo"}}}
        def fake_fetch(self, path):
            if path == "/api/states":
                raise AssertionError("Do not enumerate all entities")
            return values[path.split("/")[-1]]
        with patch.dict(os.environ, {"ALBATROS_HA_MODE": "live",
                                     "ALBATROS_HA_URL": "http://localhost:8123",
                                     "ALBATROS_HA_TOKEN": "fake",
                                     "ALBATROS_HA_ENTITIES": "sensor.allowed"}, clear=True):
            with patch("ha_readonly.HAReadOnly._fetch", fake_fetch):
                result = tool_execute(self.db, "ha_list_states", {})
                self.assertEqual([x["entity_id"] for x in result["data"]],
                                 ["sensor.allowed"])

    def test_gemini_calls_read_only_tool(self):
        with patch.dict(os.environ, {}, clear=True):
            replies = [
                '{"tool":"ha_get_state","args":{"entity_id":"sensor.albatros_demo_temperatura"}}',
                '{"answer":"Il sensore dimostrativo segnala 22,5 °C (dato simulato)."}'
            ]
            with patch("agent.model_reply", side_effect=replies) as model:
                answer = ask(self.db, "Temperatura del sensore dimostrativo?")
                self.assertIn("simulato", answer)
                self.assertEqual(model.call_count, 2)


if __name__ == "__main__":
    unittest.main()
