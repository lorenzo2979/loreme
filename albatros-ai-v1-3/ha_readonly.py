"""ALBATROS V1.5: adapter per stati Home Assistant, rigorosamente sola lettura.

Default MOCK: nessuna connessione, nessuna credenziale.
LIVE richiede opt-in esplicito + URL e token in variabili d'ambiente.
Questo modulo non invia comandi a Home Assistant e non viene invocato
automaticamente dalla chat o dall'LLM.
"""
import json
import os
import re
import urllib.error
import urllib.parse
import urllib.request

ENTITY_RE = re.compile(r"^[a-z_]+\.[a-z0-9_]+$")
MOCK_STATES = {
    "sensor.albatros_demo_temperatura": {
        "entity_id": "sensor.albatros_demo_temperatura",
        "state": "22.5",
        "attributes": {"friendly_name": "Temperatura dimostrativa", "unit_of_measurement": "°C"},
    },
    "binary_sensor.albatros_demo_porta": {
        "entity_id": "binary_sensor.albatros_demo_porta",
        "state": "off",
        "attributes": {"friendly_name": "Porta dimostrativa"},
    },
}


def _entity_id(value):
    if not isinstance(value, str) or not ENTITY_RE.fullmatch(value):
        raise ValueError("ID entità non valido")
    return value


def normalize_state(raw):
    """Conserva solo dati minimi; gli attributi possono contenere dati privati."""
    if not isinstance(raw, dict):
        raise ValueError("Risposta Home Assistant non valida")
    entity = _entity_id(raw.get("entity_id"))
    state = raw.get("state")
    if not isinstance(state, str) or len(state) > 160:
        raise ValueError("Stato non valido")
    attrs = raw.get("attributes") or {}
    if not isinstance(attrs, dict):
        raise ValueError("Attributi non validi")
    clean = {"entity_id": entity, "state": state}
    friendly = attrs.get("friendly_name")
    if isinstance(friendly, str) and len(friendly) <= 100:
        clean["name"] = friendly
    unit = attrs.get("unit_of_measurement")
    if isinstance(unit, str) and len(unit) <= 20:
        clean["unit"] = unit
    return clean


class HAReadOnly:
    def __init__(self, mode="mock", url=None, token=None, opener=None):
        if mode not in ("mock", "live"):
            raise ValueError("Modalità non consentita")
        self.mode = mode
        self.opener = opener or urllib.request.urlopen
        self.url = None
        self.token = None
        if mode == "live":
            url = url if url is not None else os.getenv("ALBATROS_HA_URL")
            token = token if token is not None else os.getenv("ALBATROS_HA_TOKEN")
            if not url or not token or not isinstance(token, str) or not token.strip():
                raise ValueError("Mancano URL o token HA nelle variabili ambiente")
            parsed = urllib.parse.urlsplit(url)
            if (parsed.scheme not in ("http", "https") or not parsed.hostname or
                    parsed.username or parsed.password or parsed.query or parsed.fragment or
                    parsed.path not in ("", "/") or parsed.port == 0):
                raise ValueError("URL Home Assistant non valido")
            if len(url) > 240 or any(c in token for c in "\r\n"):
                raise ValueError("Configurazione Home Assistant non valida")
            # Redirect HTTP disabilitati, per non inoltrare credenziali a un altro host.
            self.url = url.rstrip("/")
            self.token = token

    def _fetch(self, path):
        request = urllib.request.Request(
            self.url + path,
            headers={"Authorization": "Bearer " + self.token,
                     "Accept": "application/json"},
            method="GET")
        # urllib.request.urlopen segue i redirect implicitamente: respingiamo
        # ogni risposta il cui URL finale sia diverso dall'endpoint richiesto.
        try:
            with self.opener(request, timeout=8) as response:
                if response.geturl() != request.full_url:
                    raise ValueError("Reindirizzamento Home Assistant non consentito")
                raw = response.read(262145)
                if len(raw) > 262144:
                    raise ValueError("Risposta Home Assistant troppo grande")
                return json.loads(raw.decode("utf-8"))
        except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError) as exc:
            raise RuntimeError("Home Assistant non raggiungibile o accesso negato") from None

    def state(self, entity_id):
        entity_id = _entity_id(entity_id)
        if self.mode == "mock":
            if entity_id not in MOCK_STATES:
                raise ValueError("Entità demo non presente")
            return normalize_state(MOCK_STATES[entity_id])
        data = self._fetch("/api/states/" + entity_id)
        if data.get("entity_id") != entity_id:
            raise ValueError("La risposta non corrisponde all'entità richiesta")
        return normalize_state(data)

    def list_states(self):
        if self.mode == "mock":
            values = list(MOCK_STATES.values())
        else:
            values = self._fetch("/api/states")
        if not isinstance(values, list) or len(values) > 5000:
            raise ValueError("Elenco stati non valido")
        return [normalize_state(x) for x in values]


def main():
    import argparse
    parser = argparse.ArgumentParser(description="ALBATROS Home Assistant: sola lettura")
    parser.add_argument("--live", action="store_true",
                        help="Connette davvero Home Assistant con credenziali dall'ambiente")
    parser.add_argument("--entity", help="Legge una sola entità")
    args = parser.parse_args()
    client = HAReadOnly(mode="live" if args.live else "mock")
    result = client.state(args.entity) if args.entity else client.list_states()
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
