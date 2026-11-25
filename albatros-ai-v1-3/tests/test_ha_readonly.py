import io
import json
import unittest
from unittest.mock import patch
from ha_readonly import HAReadOnly, normalize_state


class Reply:
    def __init__(self, data, url):
        self.data = json.dumps(data).encode()
        self.url = url

    def __enter__(self): return self
    def __exit__(self, *_): pass
    def geturl(self): return self.url
    def read(self, size): return self.data[:size]


class HomeAssistantReadOnlyTests(unittest.TestCase):
    def test_mock_offline(self):
        def forbidden(*a, **kw):
            self.fail("Tentativo di rete in modalità mock")
        client = HAReadOnly(opener=forbidden)
        self.assertEqual(len(client.list_states()), 2)
        self.assertEqual(client.state("sensor.albatros_demo_temperatura")["state"], "22.5")

    def test_disallows_invalid_entity(self):
        client = HAReadOnly()
        for invalid in ("sensor.abc/../../services", "bad", "sensor.a?x=1", ""):
            with self.assertRaises(ValueError):
                client.state(invalid)

    def test_live_requires_token(self):
        with patch.dict("os.environ", {}, clear=True):
            with self.assertRaises(ValueError):
                HAReadOnly("live", url="http://localhost:8123")

    def test_rejects_url_credentials(self):
        with self.assertRaises(ValueError):
            HAReadOnly("live", url="http://user:pass@localhost:8123", token="x")

    def test_live_get_only_and_sanitized(self):
        seen = []
        data = {"entity_id": "light.demo", "state": "off",
                "attributes": {"friendly_name": "Lampada", "secret": "NON MOSTRARE"}}
        def opener(request, timeout):
            seen.append((request.get_method(), request.full_url,
                         request.get_header("Authorization"), timeout))
            return Reply(data, request.full_url)
        client = HAReadOnly("live", url="http://ha.local:8123", token="test-secret", opener=opener)
        result = client.state("light.demo")
        self.assertEqual(result, {"entity_id": "light.demo", "state": "off", "name": "Lampada"})
        self.assertEqual(seen, [("GET", "http://ha.local:8123/api/states/light.demo", "Bearer test-secret", 8)])
        self.assertNotIn("secret", str(result))

    def test_rejects_redirect(self):
        def opener(request, timeout):
            return Reply({"entity_id": "light.demo", "state": "off"}, "https://attacker.invalid/")
        with self.assertRaises(ValueError):
            HAReadOnly("live", url="http://ha.local:8123", token="fake", opener=opener).state("light.demo")

    def test_mismatched_entity(self):
        def opener(request, timeout):
            return Reply({"entity_id": "sensor.other", "state": "ok"}, request.full_url)
        with self.assertRaises(ValueError):
            HAReadOnly("live", url="http://ha.local:8123", token="fake", opener=opener).state("light.demo")

    def test_redacts_attributes(self):
        self.assertEqual(normalize_state({"entity_id": "sensor.demo", "state": "ok",
                         "attributes": {"access_code": "12345"}}),
                         {"entity_id": "sensor.demo", "state": "ok"})


if __name__ == "__main__":
    unittest.main()
